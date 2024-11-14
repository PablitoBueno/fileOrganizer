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

namespace fs = std::filesystem;
using namespace std;

// Limite de threads no pool para evitar sobrecarga
constexpr size_t MAX_THREADS = 4;

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

            sort(files.begin(), files.end());

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

// Create a window for organizing files
void createOrganizeWindow(const char* title, Fl_Callback* callback, bool keywordNeeded) {
    Fl_Window* organizeWindow = new Fl_Window(350, 200, title);
    Fl_Input* dirInput = new Fl_Input(100, 60, 200, 25, "Directory:");
    Fl_Button* browseButton = new Fl_Button(310, 60, 25, 25, "@fileopen");
    browseButton->callback([](Fl_Widget*, void* data) { openFileDialog(static_cast<Fl_Input*>(data)); }, dirInput);
    
    Fl_Input* inputField = nullptr;
    if (keywordNeeded) {
        inputField = new Fl_Input(100, 100, 200, 25, "Keyword:");
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

    mainWindow->end();
    mainWindow->show();
    return Fl::run();
}
