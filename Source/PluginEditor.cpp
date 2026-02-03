#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <cstdlib>
#include <fstream>

void debugLog(const juce::String &message) {
#if JUCE_WINDOWS
    juce::File logFile(juce::File::getSpecialLocation(juce::File::tempDirectory)
                           .getChildFile("cratedigger_debug.log"));
#else
    juce::File logFile("/tmp/cratedigger_debug.log");
#endif

    if (!logFile.exists())
        logFile.create();

    std::ofstream stream(logFile.getFullPathName().toStdString(),
                         std::ios::app);
    if (stream.is_open()) {
        juce::Time now = juce::Time::getCurrentTime();
        stream << "[" << now.toString(true, true) << "] " << message
               << std::endl;
    }

#if JUCE_WINDOWS
    OutputDebugStringA(("[CrateDigger] " + message + "\n").toRawUTF8());
#endif
}

juce::WebBrowserComponent::Options
CrateDiggerAudioProcessorEditor::getBrowserOptions() {
    auto options = juce::WebBrowserComponent::Options();
#if JUCE_WINDOWS
    debugLog("Configuring WebView2 Options");

    options =
        options
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2()
                    .withUserDataFolder(
                        juce::File::getSpecialLocation(
                            juce::File::tempDirectory)
                            .getChildFile("CrateDiggerWebView2")));

    debugLog("WebView2 Options Configured");
#elif JUCE_LINUX
    options = options.withResourceProvider(
        [](const auto &url)
            -> std::optional<juce::WebBrowserComponent::Resource> {
            return std::nullopt;
        });
#endif

    return options;
}

CrateDiggerAudioProcessorEditor::CrateDiggerAudioProcessorEditor(
    CrateDiggerAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
    setOpaque(true);

    addAndMakeVisible(downloadButton);
    downloadButton.onClick = [this] { initiateDownload(); };
    downloadButton.setEnabled(false);

    addChildComponent(dragComponent);

    dragComponent.onDragStart = [this]() {
        juce::Timer::callAfterDelay(2000, [this]() {
            dragComponent.setVisible(false);
            downloadButton.setVisible(true);

            downloadButton.setEnabled(true);
            downloadButton.setButtonText("Download This Sample");
        });
    };

    setSize(1000, 700);

    juce::MessageManager::callAsync([this]() { createWebView(); });
}

CrateDiggerAudioProcessorEditor::~CrateDiggerAudioProcessorEditor() {
    downloader.stopThread(4000);

    if (webView != nullptr) {
        debugLog("Cleaning up WebView");
        removeChildComponent(webView.get());

        webView.reset();
    }
}

void CrateDiggerAudioProcessorEditor::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::black);

    if (!isWebViewReady) {
        g.setColour(juce::Colours::white);
        g.setFont(20.0f);

        g.drawText(loadingMessage, getLocalBounds(),
                   juce::Justification::centred, true);
    }
}

void CrateDiggerAudioProcessorEditor::resized() {
    auto area = getLocalBounds();
    auto footer = area.removeFromBottom(60);

    if (webView != nullptr && isWebViewReady) {
        webView->setBounds(area);
    }

    auto centerFooter = footer.reduced(10, 5);

    downloadButton.setBounds(centerFooter);
    dragComponent.setBounds(centerFooter);
}

void CrateDiggerAudioProcessorEditor::initiateDownload() {
    if (webView == nullptr || !isWebViewReady) {
        downloadButton.setEnabled(false);
        downloadButton.setButtonText("WebView not ready");

        debugLog("Download attempted but WebView not ready");
        return;
    }

    downloadButton.setEnabled(false);
    downloadButton.setButtonText("Reading Page Title...");

    juce::String js = R"(
        (function() {
            return document.title;
        })();
    )";

    webView->evaluateJavascript(
        js, [this](juce::WebBrowserComponent::EvaluationResult result) {
            if (auto *res = result.getResult()) {
                juce::String pageTitle = res->toString();

                std::cout << "[DEBUG] Raw Page Title: " << pageTitle
                          << std::endl;

                if (pageTitle.isEmpty() || pageTitle == "Samplette" ||
                    pageTitle.contains("Loading")) {

                    downloadButton.setEnabled(true);
                    downloadButton.setButtonText("Error: Title is generic");

                    std::cout
                        << "[ERROR] Page title does not look like a song yet."
                        << std::endl;
                    return;
                }

                juce::String searchQuery = pageTitle;

                if (searchQuery.contains(" | Samplette"))
                    searchQuery = searchQuery.replace(" | Samplette", "");

                if (searchQuery.contains(" - Samplette"))
                    searchQuery = searchQuery.replace(" - Samplette", "");

                std::cout << "[SUCCESS] Search Query: " << searchQuery
                          << std::endl;

                downloadButton.setButtonText(
                    "Searching: " + searchQuery.substring(0, 15) + "...");

                juce::String command = "ytsearch1:" + searchQuery;

                downloader.download(command, [this](juce::File f) {
                    this->onDownloadFinished(f);
                });
            } else {
                downloadButton.setEnabled(true);
                downloadButton.setButtonText("JS Error");
            }
        });
}

void CrateDiggerAudioProcessorEditor::createWebView() {
    debugLog("Starting WebView creation on message thread");

#if JUCE_WINDOWS
    debugLog("Platform: Windows");
    debugLog("OS: " + juce::SystemStats::getOperatingSystemName());

	auto webview2Path = juce::File(
        "C:\\Program Files (x86)\\Microsoft\\EdgeWebView\\Application");

    if (webview2Path.exists()) {
        debugLog("WebView2 Runtime found at: " +
                 webview2Path.getFullPathName());
    } else {
        debugLog("WARNING: WebView2 Runtime not found in default location");
    }
#else
    debugLog("Platform: Linux");

    setenv("VARIABLE_NAME", "VALUE", 1);
    setenv("ANOTHER_VAR", "VALUE", 1);
#endif

    try {
        auto options = getBrowserOptions();

        debugLog("Creating WebBrowserComponent...");
        webView = std::make_unique<juce::WebBrowserComponent>(options);

        debugLog("WebBrowserComponent created, making visible...");
        addAndMakeVisible(webView.get());

        debugLog("Navigating to URL...");
        webView->goToURL("https://samplette.io");

        isWebViewReady = true;
        downloadButton.setEnabled(true);

        resized();
        repaint();

        debugLog("WebView initialized successfully");

#if JUCE_WINDOWS
        debugLog("Check log file at: " +
                 juce::File::getSpecialLocation(juce::File::tempDirectory)
                     .getChildFile("cratedigger_debug.log")
                     .getFullPathName());
#endif
    } catch (const std::exception &e) {
        loadingMessage = "Failed to load WebView: " + juce::String(e.what());
        debugLog("WebView creation failed: " + juce::String(e.what()));

        repaint();
    } catch (...) {
        loadingMessage = "Failed to load WebView: Unknown error";
        debugLog("WebView creation failed: Unknown exception");

        repaint();
    }
}

void CrateDiggerAudioProcessorEditor::onDownloadFinished(juce::File file) {
    downloadButton.setVisible(false);

    dragComponent.setFile(file);
    dragComponent.setVisible(true);
} 