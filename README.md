# Advanced Operating Systems - Assignment 1: File Reversal and Verification

## Overview

This project is made of two C++ programs that are all about manipulating files using some of the core, low-level tools in an operating system.

- **`2025201055_A1_Q1.cpp` (The File Reversal Tool):** This program’s job is to take a file and reverse its contents in one of three ways that I'll explain below. It also shows you a live progress bar so you know it's working.

- **`2025201055_A1_Q2.cpp` (The Verification Tool):** The second program is basically a fact-checker for the first one. It makes sure the file was reversed correctly, that the new folder and file have the right permissions, and that everything matches the assignment's requirements.

---

## **How It Works: My Code's Workflow**
Here’s a step-by-step explanation of how my programs work, which should make it easy to understand the logic.

### Part 1: The Program to Reverse the File (`2025201055_A1_Q1.cpp`)
Let me walk you through the process.

1.  **Kicking things off:** Everything starts in the `main` function, where the program first looks at the command you entered. It grabs the filename, the reversal mode (flag 0, 1, or 2), and any other details it needs, like a block size.

2.  **Setting up the workspace:** I created a helper function, `makeassignmentdirectory`, which handles creating the `Assignment1` folder. I  used the `mkdir()` system call and set the folder's permissions to `0700`.which makes it private, so that only  user account can access it. I also added a check so that if the folder already exists, the program won't crash;and it will just continue.

3.  **Getting the files ready:** Next, the code opens the original file in read-only mode. At the same time, it creates the new output file, naming it based on the flag (like `Assignment1/0_input.txt`), and gives it `0600` permissions for private read/write access.

4.  **The main logic - Reversing the content:** This is where the real work takes place which changes based on the flag value.
    * **For Flag 0 (`doblockwisereversal`):** First read the file in sequential chunks of the size you provide. For each chunk, I  will reverse it in memory and then immediately write the reversed chunk to the new file, continuing until I've processed the entire original file.
    * **For Flag 1 (`dofullfilereversal`):** For a complete file reversal, I decided to read the original file from the end to the beginning. As each chunk is read, it's internally reversed (to counteract being read backwards) and then written out. The result is that the last part of the original file becomes the first part of the new one.
    * **For Flag 2 (`dopartialreversal`):** This was the most challenging. I broke the problem down by thinking of the file in three distinct parts. The code reverses the first section (from the start of the file to your `start` index) and the last section (from your `end` index to the file's finish). The key part is that the middle section is copied over exactly as it was, without any changes. I accomplished all this by carefully managing the file pointers with the `lseek()` system call.

5.  **Cleaning up:** After all the writing is done, the program makes sure to close both files and release any memory that was allocated. This is important for preventing resource leaks.

### Part 2: The Verification Program (`2025201055_A1_Q2.cpp`)
My second program double-checks the work of the first one.

1.  **Reading the Command:** Just like before, it starts by reading all the file paths and the flag from your command.

2.  **Quick Checks First:** It uses a system call called `stat()` to get the file's metadata. This lets me quickly check three things: Does the `Assignment1` directory exist? Are the new and old files the exact same size?

3.  **Checking the Content:** This is where it gets detailed. Based on the flag, it calls a specific `verify_` function:
    * **For Flag 0:** It reads both files forward, one block at a time. It reverses the block from the old file and uses `memcmp()` to see if it perfectly matches the block from the new file.
    * **For Flag 1:** It reads the old file from the start and the new file from the end. It compares each piece to make sure they are mirror images of each other.
    * **For Flag 2:** It checks each of the three regions one by one to make sure the reversal rules were followed correctly for each part.

4.  **Checking Permissions:** I wrote a function `check_permissions()` that also uses `stat()` to look at the permission codes of the files and directory. It then gets a bit technical and uses some bitwise logic to look at the permission codes. This is how I can be sure that the 'user', 'group', and 'others' have the exact read, write, and execute permissions the assignment asked for.

5.  **The Final Report:** Finally, the program prints a clean "Yes/No" report that tells you if the directory was created, if the content is correct, if the sizes match, and then gives a detailed list of all 30 permission checks.

---

## Directory Structure for Submission 
```
2025201055_A1/
├── 2025201055_A1_Q1.cpp
├── 2025201055_A1_Q2.cpp
└── README.md
```

---

## Compilation and Execution
### 1. Compile the Programs:
```bash
# Compile the reversal tool
g++ 2025201055_A1_Q1.cpp -o q1_tool

# Compile the verification tool
g++ 2025201055_A1_Q2.cpp -o q2_verifier
```

### 2. Run the Reversal Tool (`q1_tool`):

* **Flag 0 (Block-wise):**
    ```bash
    ./q1_tool <input_file> 0 <block_size>
    # Example: ./q1_tool my_file.txt 0 1024
    ```

* **Flag 1 (Full Reversal):**
    ```bash
    ./q1_tool <input_file> 1
    # Example: ./q1_tool my_file.txt 1
    ```

* **Flag 2 (Partial Reversal):**
    ```bash
    ./q1_tool <input_file> 2 <start_index> <end_index>
    # Example: ./q1_tool my_file.txt 2 50 200
    ```

### 3. Run the Verification Tool (`q2_verifier`):

* **Verify Flag 0:**
    ```bash
    ./q2_verifier Assignment1/0_my_file.txt my_file.txt Assignment1 0 1024
    ```

* **Verify Flag 1:**
    ```bash
    ./q2_verifier Assignment1/1_my_file.txt my_file.txt Assignment1 1
    ```

* **Verify Flag 2:**
    ```bash
    ./q2_verifier Assignment1/2_my_file.txt my_file.txt Assignment1 2 50 200
    