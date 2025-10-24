#pragma once

#include <vector>
#include <cstdint>
#include <string>

struct SingleFontData
{
	std::vector<uint8_t> fontdata;
	std::string language;
};

class ResourceLoader
{
public:
	static std::vector<SingleFontData> LoadWidgetFontData(const std::string& name);
	static std::vector<uint8_t> LoadWidgetData(const std::string& name);
};