# cratedigger

Samplette integration in your DAW

# Disclaimer: Educational & Personal Use Only

This software is a proof-of-concept project designed for educational and personal use to demonstrate web integration with audio plugins.

- This tool does not bypass copyrights, and acts as a frontend for `yt-dlp`
- The user is solely responsible for ensuring they have the legal right to download and use any of the content processed by this software
- This software is not intended for commercial distribution or use
- Downloading from third-party platforms may violate their terms of service. Use at your own risk

# Before Building

- Copy the `Resources.zip` bundle from the `Releases` section and extract it into the `cratedigger` project directory
	- This ensures that the built application has access to yt-dlp and ffmpeg binaries

# Building on Windows 

- Use the NuGet CLI to install WebView2: `nuget install Microsoft.Web.WebView2 -OutputDirectory packages`
- Configure and specify the location of the WebView2 SDK: `cmake -B build -A x64 -DCMAKE_BUILD_TYPE=Release -DJUCE_WEBVIEW2_PACKAGE_LOCATION="C:\path\to\cratedigger\packages"`
- Build the project: `cmake --build build --config Release`