# MP3 Parser Web App (CS50 Final Project)

This is a work-in-progress MP3 parser implemented as a web application using Flask (Python) and a C backend. The core idea is to allow users to upload an MP3 file, which the server then parses to extract technical and structural information. The extracted data is then returned to the user for inspection.

The project is being developed as part of the final project for Harvard's CS50 course.

---

## Project Goals

- Build a simple web interface for users to upload MP3 files.
- Use C to efficiently parse the binary structure of MP3 files.
- Extract meaningful metadata from headers and tags.
- Return structured information about the file to the user.
- Enable future tools for inspecting, visualizing, or modifying MP3 internals.

---

## Current Features

- Skips ID3v2 tags (if present) at the start of the file.
- Validates MP3 frames based on frame header structure and finding consecutive frames.
- Web interface built with Flask for file uploads and output display.
- C program handles low-level MP3 parsing and passes results back to Python.

---

## Potential Future Features

- Show frame-by-frame breakdown.
- Detect file corruption or frame inconsistencies.
- Allow users to edit certain MP3 frame parameters.
- Improved error handling and format support (e.g., VBR headers, ID3v1).

---

## How to Run (Development)

1. Compile the C backend parser:
   ```bash
   gcc -o analyze analyze.c
