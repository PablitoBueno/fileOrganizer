# File Organizer

## Objective
This project is designed to demonstrate the application of file organization and management concepts. It provides a simple and efficient way to organize files within a directory based on different criteria such as alphabetical order and keyword matching. The primary goal is to enhance file management through automation, ensuring files are sorted and easily accessible.

## Current Features
- **Organize Alphabetically**: Sorts files within a directory into subfolders named after the first letter of the file's name (A-Z).
- **Organize by Keyword**: Moves files into subfolders based on a specific keyword present in the filename.
- **File Movement**: The program automatically moves files into corresponding folders, ensuring proper file management.

## Limitations
- The program only organizes files into subfolders, but doesn't currently support more advanced sorting methods, such as sorting by file type, size, or last modified date.
- The application doesn't handle recursive directory structures (subdirectories inside the main directory are not processed).
- It is a basic implementation and may require additional error handling for edge cases in large-scale file systems.

## Technologies Used
- **FLTK**: Used for creating the graphical user interface (GUI), providing buttons and input fields for user interaction.
- **C++ STL (Standard Template Library)**: For file system management, sorting, and vector usage.
- **Filesystem Library**: Used for navigating and manipulating the file system (creating directories, moving files).
- **Multithreading (C++11)**: For executing file organization tasks in separate threads, ensuring the GUI remains responsive.

## Practical Applications
- **File System Management**: Ideal for organizing personal files or work-related documents where a large number of files need to be sorted based on certain criteria (e.g., name or keywords).
- **Backup and Archiving**: Useful for organizing files before backing them up or archiving them, ensuring that files are categorized and easy to find.
- **Document Management Systems**: Could serve as a basic tool for managing documents in a small-scale or personal document management system.

## Future Enhancements
- **Organize by Date**: Implement functionality to sort files by their last modification date, helping users keep track of their most recent files.
- **Filter Duplicate Files**: Add the ability to identify and manage duplicate files, either by file content or by comparing file sizes and names.
- **Recursive Directory Support**: Enhance the program to handle nested directories, organizing files inside subdirectories as well.
- **Additional Sorting Options**: Include other sorting methods, such as by file type (extensions), size, or custom user-defined criteria.
- **User Customization**: Allow users to define custom sorting rules (e.g., by file extension, creation date, etc.).
- **Error Handling**: Improve error handling for edge cases and provide more detailed user feedback during file operations.

## How to Run
1. Clone the repository to your local machine:
   ```bash
   git clone https://github.com/yourusername/file-organizer.git
Ensure you have FLTK and C++11 compatible compiler installed.
Compile the project:
g++ main.cpp -o file-organizer -lfltk -std=c++11
Run the compiled application:
./file-organizer
