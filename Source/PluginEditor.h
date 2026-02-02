#pragma once

#include "PathHelper.h"
#include "PluginProcessor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <JuceHeader.h>

class YtDlpThread : public juce::Thread {
  public:
    YtDlpThread() : Thread("YtDlpThread") {}

    void download(juce::String url, std::function<void(juce::File)> callback) {
        targetUrl = url;
        onFinished = callback;
        startThread();
    }

    void run() override {
        juce::File ytDlp = PathHelper::getBinary("yt-dlp");
        juce::File ffmpeg = PathHelper::getBinary("ffmpeg");

        if (!ytDlp.exists()) {
            std::cout << "[ERROR] yt-dlp binary missing!" << std::endl;
            return;
        }

        auto tempDir =
            juce::File::getSpecialLocation(juce::File::tempDirectory);

        auto intermediateFile = tempDir.getChildFile("crate_sample");

        auto expectedFile = tempDir.getChildFile("crate_sample.wav");

        if (intermediateFile.exists())
            intermediateFile.deleteFile();

        if (expectedFile.exists())
            expectedFile.deleteFile();

        juce::StringArray args;

        args.add(ytDlp.getFullPathName());

        if (ffmpeg.exists()) {
            args.add("--ffmpeg-location");
            args.add(ffmpeg.getParentDirectory().getFullPathName());
        }

        args.add("--verbose");
        args.add("--force-overwrites");
        args.add("-x");
        args.add("--audio-format");
        args.add("wav");
        args.add("-o");
        args.add(intermediateFile.getFullPathName());

        args.add(targetUrl);

        std::cout << "[DEBUG] Executing Command: " << args.joinIntoString(" ")
                  << std::endl;

        juce::ChildProcess process;

        if (process.start(args)) {
            juce::String output = process.readAllProcessOutput();

            std::cout << "[DEBUG] Process Output:\n" << output << std::endl;

            if (expectedFile.existsAsFile() && expectedFile.getSize() > 1024) {
                std::cout << "[SUCCESS] File created at: "
                          << expectedFile.getFullPathName() << std::endl;

                juce::MessageManager::callAsync([this, expectedFile]() {
                    if (onFinished)
                        onFinished(expectedFile);
                });
            } else {
                std::cout << "[FAILURE] File not created or empty."
                          << std::endl;
            }
        } else {
            std::cout << "[ERROR] Failed to start child process." << std::endl;
        }
    }

  private:
    juce::String targetUrl;
    std::function<void(juce::File)> onFinished;
};

class DragComponent : public juce::Component {
  public:
    DragComponent() { setMouseCursor(juce::MouseCursor::DraggingHandCursor); }

    std::function<void()> onDragStart;

    void setFile(juce::File f) { fileToDrag = f; }

    void paint(juce::Graphics &g) override {
        g.fillAll(juce::Colours::darkorange);
        g.setColour(juce::Colours::white);
        g.drawText("DRAG TO DAW", getLocalBounds(),
                   juce::Justification::centred, true);
        g.drawRect(getLocalBounds(), 2);
    }

    void mouseDrag(const juce::MouseEvent &e) override {
        if (fileToDrag.existsAsFile()) {
            juce::StringArray files;

            files.add(fileToDrag.getFullPathName());

            if (auto *container =
                    juce::DragAndDropContainer::findParentDragContainerFor(
                        this)) {
                container->performExternalDragDropOfFiles(files, false, this);

                if (onDragStart)
                    onDragStart();
            }
        }
    }

  private:
    juce::File fileToDrag;
};

class CrateDiggerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::DragAndDropContainer {
  public:
    CrateDiggerAudioProcessorEditor(CrateDiggerAudioProcessor &);
    ~CrateDiggerAudioProcessorEditor() override;

    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    CrateDiggerAudioProcessor &audioProcessor;
    std::unique_ptr<juce::WebBrowserComponent> webView;

    juce::TextButton downloadButton{"⬇ Download This Sample"};
    DragComponent dragComponent;

    YtDlpThread downloader;

    void initiateDownload();
    void onDownloadFinished(juce::File file);
    void createWebView();

    static juce::WebBrowserComponent::Options getBrowserOptions();

    std::atomic<bool> isLoadingWebView{false};
    std::atomic<bool> isWebViewReady{false};
    juce::String loadingMessage = "Loading CrateDigger...";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        CrateDiggerAudioProcessorEditor)
};
