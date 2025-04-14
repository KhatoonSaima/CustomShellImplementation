# w25shell

`w25shell` is a custom shell written in C that mimics the behavior of a Linux shell with a wide range of built-in and special character operations. It is designed as a systems programming assignment for COMP-8567 (Winter 2025), focusing on OS-level process management and inter-process communication using system calls.

---

## 📌 Features

- Custom shell prompt: `w25shell$`
- Infinite loop waiting for user input
- Uses `fork()`, `exec()`, `pipe()`, `dup2()`, etc.
- **No use of `system()` function is allowed**
- Modular design encouraged

---

## 🔧 Special Commands

| Command         | Description |
|----------------|-------------|
| `killterm`     | Terminates the current `w25shell` terminal |
| `killallterms` | Terminates all `w25shell` instances currently open |

---

## ⚙️ Argument Constraints

- Each command must have `1 <= argc <= 5`
- This includes piped and compound commands

---

## 🚀 Supported Special Characters

| Character | Function |
|----------|----------|
| `|`      | Piping (up to 5 pipes allowed) |
| `=`      | Reverse Piping (up to 5 operations) |
| `~`      | Append contents of two `.txt` files to each other |
| `#`      | Count number of words in a `.txt` file |
| `+`      | Concatenate up to 5 `.txt` files in listed order |
| `<`      | Input redirection from a file |
| `>`      | Output redirection to a file |
| `>>`     | Output redirection with append to a file |
| `;`      | Sequential execution (up to 4 commands) |
| `&&`     | Conditional execution (up to 5 combined commands) |
| `||`     | Conditional OR execution (up to 5 combined commands) |

📌 Commands cannot combine special characters (e.g., `p1 & p2 > file.txt` is not allowed).

---

## 💡 Examples

```bash
w25shell$ ls -l
w25shell$ ls -l -t -a | wc -w
w25shell$ sample1.txt ~ sample2.txt
w25shell$ # sample.txt
w25shell$ wc -w = wc = ls -1
w25shell$ ls -1 > dirlist.txt
w25shell$ date ; pwd ; ls -l -t -a
w25shell$ c1 && c2 || c3 && c4
```

---

## 🧪 Testing & Environment

- Must be implemented and tested on the **CS Linux server** using your university login
- Avoid creating fork bombs: run `killall -u your_username` to clean up processes
- Plagiarism will be checked using **MOSS**

---

## 📤 Submission

1. Submit: `w25shell_fname_lname_SID.c`
2. Submit: A Zoom/Google Drive video (max 15 mins) with:
   - Code explanation (~8-9 mins)
   - Demonstration of various input cases (~6-7 mins)
   - Link must be included in comments section of code
   - Only Zoom/Drive links accepted — **no MP4 uploads**

---

## 🛠️ System Calls Used

- `fork()`
- `execvp()`
- `pipe()`
- `dup2()`
- `waitpid()`
- `kill()`
- `execlp()`

---

## 🧠 Learning Outcomes

- Apply OS concepts in Unix/Linux
- Design algorithms using system calls
- Use kernel services to solve real-world programming problems

---

## 👨‍💻 Author

Created as part of COMP-8567 Assignment — Winter 2025 (University of Windsor)
