# Qt-based-File-Explorer
## Project Overview
Qt File Explorer is a desktop application developed using C++ and Qt 6. 
The main goal of this project is to build a simple file explorer application using C++ and Qt that allows users to browse and manage files and folders through a graphical interface.
This application allows users to browse folders, manage files, and view file details in a simple and user-friendly way. 

## Main Features
 - Browse files and folders using a tree/list view

 - Open files and folders by double-clicking

 - Create new files and new folders in the current directory

 - Delete files and folders safely

 - Rename files and folders

 - Copy, cut, and paste files/folders

 - Navigate back to the previous directory

 - Keyboard shortcuts for faster file operations (such as delete, copy, paste, permanent delete and new file

 - View file and folder properties such as:
   - Name
   - Location
   - Type (file or folder)
   - Size
   - Last modified date

## Technologies Used
 - Programming Language: C++

 - Framework: Qt 6

 - Qt Modules: Widgets, QFileSystemModel, QFile, QDir

 - IDE: Qt Creator

## How to Run the Project
1.	Open Qt Creator
2.	Open the project using the .pro file
3.	Select a Qt 6 kit
4.	Build and run the application

## Design Highlights
 - Implemented using Qt Model–View architecture with QFileSystemModel to efficiently represent and manage the file system

  - Separation of concerns through modular class design, isolating main UI logic and properties dialog functionality

 - Extensive use of Qt’s signal–slot mechanism for handling user interactions and event-driven programming

 - Utilization of QFileInfo and QDir APIs for reliable file system operations and metadata retrieval

 - Recursive directory traversal for accurate folder size computation

 - Context-aware QAction-based menus integrated with toolbar and right-click context menus

 - Support for keyboard event handling to enable shortcut-driven file operations

 - Dynamic UI updates using model index mapping to reflect real-time file system changes

## Future Enhancements
 - Tab support to open multiple folders

 - Dark mode support

 - File preview for text and images

 - Drag and drop support

 - ZIP and UNZIP feature for files and folders

 - Undo and Redo support for file operations

## Screenshots

Main Window

<p align="center">
  <img src="Screenshots/main window.jpeg" width="750">
</p>

Create New File Dialog box

<p align="center">
  <img src="Screenshots/new file.jpeg" width="750">
</p>

Toggle View

<p align="center">
  <img src="Screenshots/toggle view.jpeg" width="750">
</p>

Right Click menu

<p align="center">
  <img src="Screenshots/right click menu.jpeg" width="750">
</p>

Create New Folder Dialog box

<p align="center">
  <img src="Screenshots/new folder.jpeg" width="750">
</p>

Delete operation

<p align="center">
  <img src="Screenshots/delete operation.jpeg" width="750">
</p>

Search operation

<p align="center">
  <img src="Screenshots/search operation.jpeg" width="750">
</p>

Multiple Select 

<p align="center">
  <img src="Screenshots/multiple select.jpeg" width="750">
</p>

Properties Dialog box

<p align="center">
  <img src="Screenshots/properties dialog box.jpeg" width="750">
</p>
