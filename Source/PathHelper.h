#pragma once
#include <JuceHeader.h>

#if JUCE_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class PathHelper {
  public:
    static juce::File getBinariesFolder() {
        juce::File modulePath;

#if JUCE_WINDOWS
        HMODULE hModule = NULL;

        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)&getBinariesFolder, &hModule)) {
            char path[MAX_PATH];

            if (GetModuleFileNameA(hModule, path, MAX_PATH) != 0) {
                modulePath = juce::File(path);
            }
        }

#else
        Dl_info dl_info;

        if (dladdr((void *)&getBinariesFolder, &dl_info) != 0) {
            modulePath = juce::File(juce::String(dl_info.dli_fname));
        }
#endif
        if (modulePath == juce::File())
            modulePath = juce::File::getSpecialLocation(
                juce::File::currentExecutableFile);

        auto siblingBin = modulePath.getSiblingFile("CrateDigger_Binaries");

        if (siblingBin.isDirectory())
            return siblingBin;

        auto parent = modulePath.getParentDirectory();

        for (int i = 0; i < 5; ++i) {
            auto check = parent.getChildFile("CrateDigger_Binaries");

            if (check.isDirectory())
                return check;

            parent = parent.getParentDirectory();

            if (parent == parent.getParentDirectory())
                break;
        }

        std::cout << "[PathHelper] Warning: Could not find "
                     "CrateDigger_Binaries folder relative to: "
                  << modulePath.getFullPathName() << std::endl;

        return juce::File();
    }

    static juce::File getBinary(const juce::String &name) {
#if JUCE_WINDOWS
        return getBinariesFolder().getChildFile(name + ".exe");
#else
        return getBinariesFolder().getChildFile(name);
#endif
    }
};
