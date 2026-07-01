# StudyFlash

A simple SDL2 + SDL2_ttf C++ flashcard application.

## Prerequisites

### Windows

This project uses:

- C++ (g++)
- SDL2
- SDL2_ttf
- MSYS2 UCRT64

---

## 1. Install MSYS2

Download and install MSYS2:

https://www.msys2.org/

Open:

```text
MSYS2 UCRT64
```

---

## 2. Update MSYS2

Run:

```bash
pacman -Syu
```

Close the terminal if prompted and reopen UCRT64.

Run again:

```bash
pacman -Su
```

---

## 3. Install C++ Compiler

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
```

Verify:

```bash
g++ --version
```

You should see a version number.

---

## 4. Install SDL2

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2
```

Verify:

```bash
sdl2-config --version
```

---

## 5. Install SDL2_ttf

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
```

Verify:

```bash
pacman -Qs SDL2_ttf
```

Expected output:

```text
local/mingw-w64-ucrt-x86_64-SDL2_ttf
```

---

## 6. Clone Repository

```bash
git clone <repository-url>
cd Flashcardapp
```

---

## 7. Verify Fonts

The project requires:

```text
DejaVuSans.ttf
DejaVuSans-Bold.ttf
```

Ensure these files exist in the project root:

```text
Flashcardapp/
├── studyflashapp.cxx
├── DejaVuSans.ttf
├── DejaVuSans-Bold.ttf
└── ...
```

---

## 8. Build

Open the **MSYS2 UCRT64** terminal in the project folder.

Compile:

```bash
g++ studyflashapp.cxx -o studyflashapp $(sdl2-config --cflags --libs) -lSDL2_ttf -std=c++17
```

---

## 9. Run

```bash
./studyflashapp
```

---

# VS Code Setup (Optional)

Install:

- C/C++ Extension (Microsoft)

Compiler path:

```text
C:\msys64\ucrt64\bin\g++.exe
```

Include paths:

```text
C:\msys64\ucrt64\include
C:\msys64\ucrt64\include\SDL2
```

---

# Troubleshooting

## g++ not recognized

Check:

```bash
g++ --version
```

If not found:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc
```

Or restart VS Code after updating PATH.

---

## SDL2 headers not found

Example:

```text
cannot open source file "SDL2/SDL.h"
```

Install SDL2:

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2
```

---

## SDL2_ttf headers not found

Example:

```text
cannot open source file "SDL2/SDL_ttf.h"
```

Install:

```bash
pacman -S mingw-w64-ucrt-x86_64-SDL2_ttf
```

Verify:

```bash
ls /ucrt64/include/SDL2/SDL_ttf.h
```

---

## undefined reference to SDL_main

Make sure your entry point is:

```cpp
int main(int argc, char* argv[])
{
    // code
}
```

Then rebuild.

---

## Font load failed

Example:

```text
Font load failed
```

Verify required font files exist:

```bash
ls DejaVuSans.ttf
ls DejaVuSans-Bold.ttf
```

---

## sdl2-config not working in PowerShell

The build command uses:

```bash
$(sdl2-config --cflags --libs)
```

which is a Bash/MSYS2 feature.

Use the **MSYS2 UCRT64** terminal to build the project.

---

## App won't stop

Close the SDL window or press:

```text
Ctrl + C
```

If needed:

```powershell
taskkill /F /IM studyflashapp.exe
```

---

# Clean Build

Remove previous executable:

```bash
rm studyflashapp.exe
```

Rebuild:

```bash
g++ studyflashapp.cxx -o studyflashapp $(sdl2-config --cflags --libs) -lSDL2_ttf -std=c++17
```

---

# Verify Installation

```bash
g++ --version
sdl2-config --version
pacman -Qs SDL2_ttf
```

All commands should return valid output before building the project.
