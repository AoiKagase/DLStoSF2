#include "dls2sf2.hpp"

namespace dls2sf2 {

void Dls2SF2::convert(const std::string& inputPath, const std::string& outputPath) {
    const DlsFile dls = parser_.parse(inputPath);
    const SF2File sf2 = converter_.convert(dls);
    writer_.write(outputPath, sf2);
}

}  // namespace dls2sf2
