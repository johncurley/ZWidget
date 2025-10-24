#include "core/resourcedata.h"
#include <fstream>
#include <stdexcept>
#include <iostream>

std::vector<SingleFontData> ResourceLoader::LoadWidgetFontData(const std::string& name)
{
    std::vector<SingleFontData> fonts;

    // For Linux, we'll try to load OpenSans.ttf as a fallback.
    // In a real application, you might want to query system fonts or have a more robust fallback mechanism.
    if (name == "SystemDefault")
    {
        std::string fontPath = "/Users/johncurley/Projects/gzdoom/libraries/ZWidget/example/OpenSans.ttf";
        std::ifstream file(fontPath, std::ios::binary | std::ios::ate);

        if (file.is_open())
        {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);

            SingleFontData fontData;
            fontData.fontdata.resize(size);
            if (file.read(reinterpret_cast<char*>(fontData.fontdata.data()), size))
            {
                fontData.language = "en"; // Assuming English for OpenSans
                fonts.push_back(std::move(fontData));
                std::cout << "Loaded OpenSans.ttf as SystemDefault font." << std::endl;
            }
            else
            {
                std::cerr << "Failed to read OpenSans.ttf: " << fontPath << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to open OpenSans.ttf: " << fontPath << std::endl;
        }
    }
    // Add other font loading logic here if needed for other font names

    if (fonts.empty())
    {
        throw std::runtime_error("Could not load font: " + name);
    }

    return fonts;
}

std::vector<uint8_t> ResourceLoader::LoadWidgetData(const std::string& name)
{
    // This is a placeholder. In a real scenario, this would load other resources like images.
    // For now, we'll just return an empty vector or throw an error if a resource is requested.
    std::string resourcePath = "/Users/johncurley/Projects/gzdoom/libraries/ZWidget/example/" + name;
    std::ifstream file(resourcePath, std::ios::binary | std::ios::ate);

    if (file.is_open())
    {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        if (file.read(reinterpret_cast<char*>(data.data()), size))
        {
            return data;
        }
        else
        {
            std::cerr << "Failed to read resource: " << resourcePath << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to open resource: " << resourcePath << std::endl;
    }
    return {};
}
