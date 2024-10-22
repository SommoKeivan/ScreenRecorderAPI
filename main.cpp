#if __linux__
#include <unistd.h> // Include for Linux sleep functionality
#endif
#include <iostream>
#include "include/ScreenRecorder.h" // Include the ScreenRecorder header

int main(int argc, const char* argv[]) {
    ScreenRecorder screenRecorder; // Create a ScreenRecorder object
    std::cout << "---- Starting ScreenRecorder ----" << std::endl; // Message indicating start

    int width, height, x, y; // Variables for screen resolution and offset
    do {
        // Prompt user for screen resolution
        std::cout << "Enter resolution (width x height): ";
        std::cin >> width >> height;

        // Prompt user for screen offset
        std::cout << "Enter offset (x, y): ";
        std::cin >> x >> y;
    } while (!screenRecorder.set(true, width, height, x, y)); // Set screen recorder parameters

    screenRecorder.start(); // Start the screen recording

#if WIN32
    Sleep(8000); // Sleep for 8 seconds on Windows
#else
    sleep(8); // Sleep for 8 seconds on Linux
#endif

    screenRecorder.pause(); // Pause the screen recording
    std::cout << "---- Pausing ScreenRecorder ----" << std::endl; // Message indicating pause

#if WIN32
    Sleep(5000); // Sleep for 5 seconds on Windows
#else
    sleep(5); // Sleep for 5 seconds on Linux
#endif

    screenRecorder.resume(); // Resume the screen recording
    std::cout << "---- Resuming ScreenRecorder ----" << std::endl; // Message indicating resume

#if WIN32
    Sleep(3000); // Sleep for 3 seconds on Windows
#else
    sleep(3); // Sleep for 3 seconds on Linux
#endif

    screenRecorder.stop(); // Stop the screen recording
    std::cout << "---- Stopping ScreenRecorder ----" << std::endl; // Message indicating stop

    return 0; // Return 0 indicating successful execution
}
