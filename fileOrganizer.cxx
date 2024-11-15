#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_File_Chooser.H>
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <future>
#include <sstream>

namespace fs = std::filesystem;
using namespace std;

// Thread limit in the pool to prevent overload
constexpr size_t MAX_THREADS = 4;

// Heuristic to evaluate the "ease" of moving a file
// Prioritizes smaller files, with shorter paths and specific extensions
class FileHeuristic {
public:
    static int evaluate(const fs::path& file) {
        if (fs::is_regular_file(file)) {
            auto size = fs::file_size(file);
            auto pathLength = file.string().length();
            auto ext = file.extension().string();
            
            // Smaller files and shorter paths have higher priority
            int priority = size + pathLength;
            
            // Prioritize specific extensions (e.g., images, texts, etc.)
            if (ext == ".txt" || ext == ".jpg" || ext == ".png") {
                priority -= 1000; // Decrease the value to prioritize these types
            }

            return priority;
        }
        return 0;
    }
};

// Thread pool
class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); });
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers) {
            worker.join();
        }
    }

    template<class F>
    void enqueue(F&& f) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace(std::forward<F>(f));
        }
        condition.notify_one();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
};

// FileOrganizer class handles file operations
class FileOrganizer {
public:
    // Display a message box with a given message
    static void showMessage(const string& message) {
        Fl::warning(message.c_str());
    }

    // Create a directory if it doesn't exist
    static bool createDirectory(const fs::path& path) {
        try {
            if (!fs::exists(path)) {
                fs::create_directory(path);
            }
            return true;
        } catch (const exception& e) {
            showMessage("Error creating directory: " + path.string());
            return false;
        }
    }

    // Move a file from source to destination
    static bool moveFile(const fs::path& source, const fs::path& destination) {
        try {
            fs::rename(source, destination);
            return true;
        } catch (const exception& e) {
            showMessage("Error moving file: " + source.string() + " to " + destination.string());
            return false;
        }
    }

    // Organize files alphabetically by the first letter of the filename
    static void organizeAlphabetically(const fs::path& dir, ThreadPool& pool) {
        if (dir.empty() || !fs::exists(dir)) {
            showMessage("Invalid directory.");
            return;
        }

        vector<fs::path> files;
        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }

            if (files.empty()) {
                showMessage("No files found.");
                return;
            }

            // Sort the files according to the heuristic
            sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
                return FileHeuristic::evaluate(a) < FileHeuristic::evaluate(b);  // Prioritize files with smaller heuristics
            });

            // Create subdirectories for each letter (A-Z) and move files
            for (char letter = 'A'; letter <= 'Z'; ++letter) {
                fs::path letterFolder = dir / string(1, letter);
                if (!createDirectory(letterFolder)) {
                    continue;
                }

                // Enqueue tasks for moving files using the thread pool
                for (const auto& file : files) {
                    char firstChar = toupper(file.filename().string()[0]);
                    if (firstChar == letter) {
                        pool.enqueue([=]() {
                            if (!moveFile(file, letterFolder / file.filename())) {
                                showMessage("Error moving file: " + file.string());
                            }
                        });
                    }
                }
            }

            showMessage("Files organized alphabetically!");
        } catch (const exception& e) {
            showMessage("Error organizing files alphabetically.");
        }
    }

    // Organize files based on a given keyword in the filename
    static void organizeByKeyword(const fs::path& dir, const string& keyword, ThreadPool& pool) {
        if (dir.empty() || !fs::exists(dir) || keyword.empty()) {
            showMessage("Invalid directory or keyword.");
            return;
        }

        vector<fs::path> files;
        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }

            if (files.empty()) {
                showMessage("No files found.");
                return;
            }

            fs::path keywordFolder = dir / keyword;
            if (!createDirectory(keywordFolder)) {
                return;
            }

            // Sort the files according to the heuristic
            sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
                return FileHeuristic::evaluate(a) < FileHeuristic::evaluate(b);  // Prioritize files with smaller heuristics
            });

            // Enqueue files with names containing the keyword
            for (const auto& file : files) {
                if (file.filename().string().find(keyword) != string::npos) {
                    pool.enqueue([=]() {
                        if (!moveFile(file, keywordFolder / file.filename())) {
                            showMessage("Error moving file: " + file.string());
                        }
                    });
                }
            }

            showMessage("Files with keyword '" + keyword + "' moved successfully!");
        } catch (const exception& e) {
            showMessage("Error organizing files by keyword.");
        }
    }

    // Organize files based on content, searching for keywords inside the files
    static void organizeByContent(const fs::path& dir, const vector<string>& keywords, ThreadPool& pool) {
        if (dir.empty() || !fs::exists(dir)) {
            showMessage("Invalid directory.");
            return;
        }

        vector<fs::path> files;
        try {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    files.push_back(entry.path());
                }
            }

            if (files.empty()) {
                showMessage("No files found.");
                return;
            }

            // Sort the files according to the heuristic
            sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
                return FileHeuristic::evaluate(a) < FileHeuristic::evaluate(b);  // Prioritize files with smaller heuristics
            });

            for (const auto& file : files) {
                string content = getFileContent(file);

                // Check if the content contains any of the keywords
                for (const auto& keyword : keywords) {
                    if (content.find(keyword) != string::npos) {
                        fs::path keywordFolder = dir / keyword;
                        if (!createDirectory(keywordFolder)) {
                            continue;
                        }

                        pool.enqueue([=]() {
                            if (!moveFile(file, keywordFolder / file.filename())) {
                                showMessage("Error moving file: " + file.string());
                            }
                        });
                        break;
                    }
                }
            }

            showMessage("Files organized based on content!");
        } catch (const exception& e) {
            showMessage("Error organizing files by content.");
        }
    }

    // Read file content as a string
    static string getFileContent(const fs::path& file) {
        ifstream inputFile(file);
        stringstream buffer;
        buffer << inputFile.rdbuf();
        return buffer.str();
    }
};

// Open file dialog to choose a directory
void openFileDialog(Fl_Input* inputField) {
    const char* path = fl_dir_chooser("Choose a directory", "");
    if (path != nullptr) {
        inputField->value(path);
    }
}

// Callback for organizing files alphabetically
void organizeAlphabeticallyCallback(Fl_Widget*, void* data) {
    Fl_Input* dirInput = static_cast<Fl_Input*>(data);
    string dir = dirInput->value();
    if (dir.empty()) {
        Fl::warning("Please, enter the directory.");
        return;
    }
    ThreadPool pool(MAX_THREADS);  // Create a thread pool
    FileOrganizer::organizeAlphabetically(dir, pool);
}

// Callback for organizing files by keyword
void organizeByKeywordCallback(Fl_Widget*, void* data) {
    pair<Fl_Input*, Fl_Input*>* inputs = static_cast<pair<Fl_Input*, Fl_Input*>*>(data);
    string dir = inputs->first->value();
    string keyword = inputs->second->value();
    if (dir.empty() || keyword.empty()) {
        Fl::warning("Please, enter the directory and keyword.");
        return;
    }
    ThreadPool pool(MAX_THREADS);  // Create a thread pool
    FileOrganizer::organizeByKeyword(dir, keyword, pool);
}

// Callback for organizing files by content
void organizeByContentCallback(Fl_Widget*, void* data) {
    pair<Fl_Input*, Fl_Input*>* inputs = static_cast<pair<Fl_Input*, Fl_Input*>*>(data);
    string dir = inputs->first->value();
    string keywordsInput = inputs->second->value();
    if (dir.empty() || keywordsInput.empty()) {
        Fl::warning("Please, enter the directory and keywords.");
        return;
    }

    // Split the keywords input into a vector of keywords
    vector<string> keywords;
    stringstream ss(keywordsInput);
    string keyword;
    while (getline(ss, keyword, ',')) {
        keywords.push_back(keyword);
    }

    ThreadPool pool(MAX_THREADS);  // Create a thread pool
    FileOrganizer::organizeByContent(dir, keywords, pool);
}

// Create a window for organizing files
void createOrganizeWindow(const char* title, Fl_Callback* callback, bool keywordNeeded) {
    Fl_Window* organizeWindow = new Fl_Window(350, 200, title);
    Fl_Input* dirInput = new Fl_Input(100, 60, 200, 25, "Directory:");
    Fl_Button* browseButton = new Fl_Button(310, 60, 25, 25, "@fileopen");
    browseButton->callback([](Fl_Widget*, void* data) { openFileDialog(static_cast<Fl_Input*>(data)); }, dirInput);
    
    Fl_Input* inputField = nullptr;
    if (keywordNeeded) {
        inputField = new Fl_Input(100, 100, 200, 25, " Content:");
    }

    Fl_Button* okButton = new Fl_Button(100, 150, 200, 30, "Organize");
    okButton->callback(callback, keywordNeeded ? static_cast<void*>(new pair<Fl_Input*, Fl_Input*>(dirInput, inputField)) : static_cast<void*>(dirInput));

    organizeWindow->end();
    organizeWindow->show();
}

// Main function to create the main window and buttons
int main() {
    Fl_Window* mainWindow = new Fl_Window(400, 300, "File Organizer");
    Fl_Button* btnAlphabetical = new Fl_Button(50, 50, 300, 40, "Organize Alphabetically");
    btnAlphabetical->callback([](Fl_Widget*, void*) {
        createOrganizeWindow("Organize Alphabetically", organizeAlphabeticallyCallback, false);
    });
    Fl_Button* btnKeyword = new Fl_Button(50, 100, 300, 40, "Organize by Keyword");
    btnKeyword->callback([](Fl_Widget*, void*) {
        createOrganizeWindow("Organize by Keyword", organizeByKeywordCallback, true);
    });
    Fl_Button* btnContent = new Fl_Button(50, 150, 300, 40, "Organize by Content");
    btnContent->callback([](Fl_Widget*, void*) {
        createOrganizeWindow("Organize by Content", organizeByContentCallback, true);
    });

    mainWindow->end();
    mainWindow->show();
    return Fl::run();
}
