# File Organizer

## Objective
This project demonstrates file organization and management concepts. It provides a simple and efficient way to organize files within a directory based on different criteria, such as alphabetical order and keyword matching. The primary goal is to automate file organization, ensuring files are sorted and easily accessible.

## Algorithm Description
The core of the `File Organizer` application is a multithreaded algorithm designed to sort and organize files in a specified directory. Here's a breakdown of the algorithm used:

### 1. **Directory Scanning**
The algorithm starts by scanning a directory specified by the user. Using the C++17 **filesystem library**, it collects all the files within the directory. The program only processes regular files (ignoring directories).

### 2. **File Evaluation (Heuristic Scoring)**
Each file is evaluated based on a heuristic score to prioritize files that are easier to move. The heuristic evaluates:
- **File Size**: Smaller files are given a higher priority as they are quicker to move.
- **Path Length**: Files with shorter paths are prioritized for movement.
- **File Extensions**: Certain file extensions (like `.txt`, `.jpg`, `.png`, etc.) are prioritized and given a higher weight in the heuristic evaluation. These file types are often smaller and more common in typical file systems.

The evaluation is performed using a function `FileHeuristic::evaluate()`, which returns an integer score. Files with lower heuristic scores are prioritized for organization.

### 3. **Multithreading (ThreadPool)**
To improve performance, the program uses a **ThreadPool** that runs multiple threads concurrently. This allows files to be moved in parallel, speeding up the file organization process. The pool size is configurable, but the default is set to `4` threads, which strikes a balance between performance and resource utilization.

- **Thread Pool Initialization**: A thread pool is created at the beginning, and tasks are enqueued for each file move operation.
- **File Move Task**: Each thread picks up a task to move a file into its corresponding directory.
- **Task Completion**: Once all tasks are completed, the program finishes the operation, ensuring the GUI remains responsive throughout.

### 4. **File Organization Strategies**
There are multiple ways the user can organize their files:

#### - **Alphabetical Organization**
In this mode, files are sorted based on the first letter of their name (A-Z). Files are moved into subdirectories named after the corresponding letter of the alphabet. For example, all files starting with the letter "A" are moved into the subfolder `A/`, all files starting with "B" are moved into `B/`, and so on.

#### - **Keyword-Based Organization**
In this mode, the user provides a keyword. The program then moves files that contain the keyword in their filename into a subdirectory named after the keyword. For example, if the keyword is `invoice`, all files with `invoice` in their name are moved into the `invoice/` folder.

#### - **Content-Based Organization**
For this mode, the program searches for specific keywords within the content of the files. Users can provide a list of keywords, and the program will examine the contents of each file. If a file contains any of the specified keywords, it is moved into a subdirectory named after that keyword.

### 5. **File Movement**
Once files are categorized into their respective groups (alphabetical, by keyword, or by content), each file is moved into the corresponding subfolder. The program ensures that:
- Subdirectories are created if they do not already exist.
- Files are moved without overwriting existing files (the program performs checks to prevent conflicts).
- Errors are handled gracefully, and the user is notified if any issues arise during file movement.

### 6. **Error Handling and User Feedback**
The program provides error messages and warnings using the **FLTK** message boxes to inform the user of any issues that occur during file operations (e.g., if a file can't be moved or a directory couldn't be created).

---

## Current Features
- **Organize Alphabetically**: Sorts files within a directory into subfolders named after the first letter of the file's name (A-Z).
- **Organize by Keyword**: Moves files into subfolders based on a specific keyword present in the filename.
- **Organize by Content**: Organizes files by searching for keywords inside the file content.
- **Multithreaded File Movement**: The program automatically moves files into corresponding folders concurrently, improving performance and ensuring that the GUI remains responsive during file operations.

## Limitations
- The program only organizes files into subfolders, but doesn't currently support advanced sorting methods such as by file type, size, or last modified date.
- The application does not handle recursive directory structures (i.e., subdirectories inside the main directory are not processed).
- It is a basic implementation and may require additional error handling for edge cases in large-scale file systems.

## Technologies Used
- **FLTK**: The graphical user interface (GUI) framework used for providing buttons and input fields for user interaction.
- **C++ STL (Standard Template Library)**: Used for file system management, sorting, and vector usage.
- **Filesystem Library (C++17)**: Used for navigating and manipulating the file system (creating directories, moving files).
- **Multithreading (C++11)**: For executing file organization tasks in separate threads, ensuring the GUI remains responsive.

## Practical Applications
- **File System Management**: Ideal for organizing personal files or work-related documents where a large number of files need to be sorted based on name or keywords.
- **Backup and Archiving**: Useful for organizing files before backing them up or archiving them, ensuring that files are categorized and easy to find.
- **Document Management Systems**: Can serve as a basic tool for managing documents in small-scale or personal document management systems.

## Future Enhancements
- **Organize by Date**: Implement functionality to sort files by their last modification date, helping users keep track of their most recent files.
- **Filter Duplicate Files**: Add the ability to identify and manage duplicate files, either by file content or by comparing file sizes and names.
- **Recursive Directory Support**: Enhance the program to handle nested directories, organizing files inside subdirectories as well.
- **Additional Sorting Options**: Include other sorting methods, such as by file type (extensions), size, or custom user-defined criteria.
- **User Customization**: Allow users to define custom sorting rules (e.g., by file extension, creation date, etc.).
- **Error Handling**: Improve error handling for edge cases and provide more detailed user feedback during file operations.

## How to Run

1. **Clone the repository** to your local machine:
   ```bash
   git clone https://github.com/yourusername/file-organizer.git
   
2. **Ensure you have FLTK and a C++11 compatible compiler installed**:
   - **FLTK**: You can install FLTK by following the instructions on the FLTK website.
   - **C++11 Compiler**: Ensure your compiler supports C++11. Most modern C++ compilers do (e.g., GCC, Clang).

3. **Compile the project**:
   ```bash
   g++ main.cpp -o file-organizer -lfltk -std=c++11

4. **Run the compiled application**:
```bash
./fileOrganizer
