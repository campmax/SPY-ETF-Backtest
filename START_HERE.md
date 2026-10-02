# Start here — Visual Studio on Windows

## Simplest: use the console project you already created

1. Extract **all** files from `QuantLab.zip` into a normal folder.
2. In Visual Studio's **Solution Explorer → Source Files**, open your existing
   `.cpp` file (for example `Backtester.cpp`).
3. Open the supplied `QuantLab/Backtester.cpp` in a text editor. Copy its entire
   contents. Replace the entire contents of your project's `.cpp` file with it.
   Keep only one file containing `main()` in this project. Do not also add
   `src/main.cpp` or the tests to that same console project.
4. Right-click your **project** (not the solution) → **Properties**. Select
   **All Configurations**, then **C/C++ → Language → C++ Language Standard →
   ISO C++17 Standard (/std:c++17)**. Click Apply.
5. Choose **Release** and **x64** in the toolbar. Press **Ctrl+F5**.
6. The program runs a **synthetic demonstration** and prints the full path to
   `report.html`. Open that file in a browser. These are invented prices, not SPY.

If you see a precompiled-header error mentioning `pch.h`, set **C/C++ →
Precompiled Headers → Not Using Precompiled Headers**. A basic Console App
normally does not require this.

To run historical data in this existing project, set **Properties → Debugging →
Command Arguments** to the desired options, using absolute paths, for example:

```text
--data "C:\QuantLab\data\SPY.csv" --start-date 2002-01-01 --holdout-date 2021-01-01 --phase development --out "C:\QuantLab\results\first_spy_run"
```

Adjust paths to where you extracted the project. A new output folder is required
each time. The program reports errors directly in the console.

## Complete project: open the supplied folder

This is preferable as you begin editing and studying the code.

1. In Visual Studio Installer, install **Desktop development with C++**, including
   the CMake tools and Windows SDK.
2. In Visual Studio choose **File → Open → Folder**, then the extracted `QuantLab`
   folder containing `CMakeLists.txt`. Let configuration finish.
3. Select **quantlab.exe** as the startup target, then **Ctrl+F5**.
4. The tests are a separate target called **quantlab_tests.exe**. Select and run it
   to verify the accounting, then switch back to quantlab.exe.

Visual Studio uses its CMake build location; it may differ from the `build` folder
shown in command examples. The alternative below creates that exact folder.

## Build and launch without configuring a project

From a Windows terminal in the extracted `QuantLab` folder:

```powershell
.\build_windows.cmd
.\run_demo.cmd
```

The build script locates Visual Studio's C++ tools, compiles the engine and tests,
and runs the tests. The demo launcher opens the HTML report after a successful run.
It can also be double-clicked. These scripts are supplied for convenience; they
were not executed on Windows in the development environment.

## Get actual SPY history

Install Python if you do not have it. In a terminal in this folder:

```powershell
py -m pip install yfinance
py tools\fetch_spy.py
```

This downloads 2000 through 2025 by default and writes `data/SPY.csv`. It requires
internet access and can be blocked/rate-limited by the provider. A failure is not
a successful backtest: do not substitute invented data and label it SPY.

Then follow the commands in `README.md`. First open only the **development**
period. Read `docs/RESEARCH_PLAN.md` before opening the holdout.

## What to send back for the next research step

For setup trouble: the exact error text or a screenshot of the full console.
For strategy review: the **development** `summary.csv`, `run_manifest.txt`, and
your experiment notes. Avoid sending or opening holdout results until rules are
frozen. Never send brokerage credentials or API secrets.
