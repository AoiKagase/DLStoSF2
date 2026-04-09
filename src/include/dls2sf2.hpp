#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace dls2sf2 {

constexpr uint32_t makeFourCC(char a, char b, char c, char d) {
    return static_cast<uint32_t>(static_cast<uint8_t>(a)) |
           (static_cast<uint32_t>(static_cast<uint8_t>(b)) << 8) |
           (static_cast<uint32_t>(static_cast<uint8_t>(c)) << 16) |
           (static_cast<uint32_t>(static_cast<uint8_t>(d)) << 24);
}

constexpr uint32_t RIFF_ID = makeFourCC('R', 'I', 'F', 'F');
constexpr uint32_t LIST_ID = makeFourCC('L', 'I', 'S', 'T');

constexpr uint32_t DLS_ID = makeFourCC('D', 'L', 'S', ' ');
constexpr uint32_t WAVE_LIST_ID = makeFourCC('w', 'v', 'p', 'l');
constexpr uint32_t WAVE_ID = makeFourCC('w', 'a', 'v', 'e');
constexpr uint32_t LINS_ID = makeFourCC('l', 'i', 'n', 's');
constexpr uint32_t INS_ID = makeFourCC('i', 'n', 's', ' ');
constexpr uint32_t LRGN_ID = makeFourCC('l', 'r', 'g', 'n');
constexpr uint32_t RGN_ID = makeFourCC('r', 'g', 'n', ' ');
constexpr uint32_t RGN2_ID = makeFourCC('r', 'g', 'n', '2');
constexpr uint32_t LART_ID = makeFourCC('l', 'a', 'r', 't');
constexpr uint32_t LAR2_ID = makeFourCC('l', 'a', 'r', '2');
constexpr uint32_t ART1_ID = makeFourCC('a', 'r', 't', '1');
constexpr uint32_t COLH_ID = makeFourCC('c', 'o', 'l', 'h');
constexpr uint32_t PTBL_ID = makeFourCC('p', 't', 'b', 'l');
constexpr uint32_t INSH_ID = makeFourCC('i', 'n', 's', 'h');
constexpr uint32_t RGNH_ID = makeFourCC('r', 'g', 'n', 'h');
constexpr uint32_t WSMP_ID = makeFourCC('w', 's', 'm', 'p');
constexpr uint32_t WTPT_ID = makeFourCC('w', 't', 'p', 't');
constexpr uint32_t WLNK_ID = makeFourCC('w', 'l', 'n', 'k');
constexpr uint32_t FMT_ID = makeFourCC('f', 'm', 't', ' ');
constexpr uint32_t DATA_ID = makeFourCC('d', 'a', 't', 'a');
constexpr uint32_t VERS_ID = makeFourCC('v', 'e', 'r', 's');
constexpr uint32_t INFO_ID = makeFourCC('I', 'N', 'F', 'O');
constexpr uint32_t INAM_ID = makeFourCC('I', 'N', 'A', 'M');
constexpr uint32_t ICRD_ID = makeFourCC('I', 'C', 'R', 'D');
constexpr uint32_t IENG_ID = makeFourCC('I', 'E', 'N', 'G');
constexpr uint32_t ICMT_ID = makeFourCC('I', 'C', 'M', 'T');
constexpr uint32_t ICOP_ID = makeFourCC('I', 'C', 'O', 'P');
constexpr uint32_t ISFT_ID = makeFourCC('I', 'S', 'F', 'T');

constexpr uint32_t SFBK_ID = makeFourCC('s', 'f', 'b', 'k');
constexpr uint32_t SDTA_ID = makeFourCC('s', 'd', 't', 'a');
constexpr uint32_t PDTA_ID = makeFourCC('p', 'd', 't', 'a');
constexpr uint32_t IFIL_ID = makeFourCC('i', 'f', 'i', 'l');
constexpr uint32_t ISNG_ID = makeFourCC('i', 's', 'n', 'g');
constexpr uint32_t IROM_ID = makeFourCC('i', 'r', 'o', 'm');
constexpr uint32_t PHDR_ID = makeFourCC('p', 'h', 'd', 'r');
constexpr uint32_t PBAG_ID = makeFourCC('p', 'b', 'a', 'g');
constexpr uint32_t PMOD_ID = makeFourCC('p', 'm', 'o', 'd');
constexpr uint32_t PGEN_ID = makeFourCC('p', 'g', 'e', 'n');
constexpr uint32_t INST_ID = makeFourCC('i', 'n', 's', 't');
constexpr uint32_t IBAG_ID = makeFourCC('i', 'b', 'a', 'g');
constexpr uint32_t IMOD_ID = makeFourCC('i', 'm', 'o', 'd');
constexpr uint32_t IGEN_ID = makeFourCC('i', 'g', 'e', 'n');
constexpr uint32_t SHDR_ID = makeFourCC('s', 'h', 'd', 'r');
constexpr uint32_t SMPL_ID = makeFourCC('s', 'm', 'p', 'l');

struct DlsInfo {
    std::string name;
    std::string creationDate;
    std::string engineer;
    std::string comment;
    std::string copyright;
};

struct DlsPoolTable {
    uint32_t cbSize = 8;
    std::vector<uint32_t> cues;
};

struct DlsWaveLoop {
    uint32_t cbSize = 16;
    uint32_t loopType = 0;
    uint32_t loopStart = 0;
    uint32_t loopLength = 0;
};

struct DlsWaveSampleInfo {
    uint32_t cbSize = 20;
    uint16_t unityNote = 60;
    int16_t fineTune = 0;
    int32_t attenuation = 0;
    uint32_t options = 0;
    uint32_t loopCount = 0;
    std::vector<DlsWaveLoop> loops;
};

struct DlsWaveLink {
    uint16_t options = 0;
    uint16_t phaseGroup = 0;
    uint32_t channel = 1;
    uint32_t tableIndex = 0;
};

struct DlsConnection {
    uint16_t source = 0;
    uint16_t control = 0;
    uint16_t destination = 0;
    uint16_t transform = 0;
    int32_t scale = 0;
};

struct DlsArticulation {
    uint32_t cbSize = 8;
    uint32_t connectionCount = 0;
    std::vector<DlsConnection> connections;
};

struct DlsRegionHeader {
    uint16_t keyLow = 0;
    uint16_t keyHigh = 127;
    uint16_t velocityLow = 0;
    uint16_t velocityHigh = 127;
    uint16_t options = 0;
    uint16_t keyGroup = 0;
};

struct DlsWave {
    std::string name;
    uint16_t formatTag = 1;
    uint16_t channels = 1;
    uint32_t sampleRate = 44100;
    uint16_t bitDepth = 16;
    std::vector<int16_t> sampleData;
    std::optional<DlsWaveSampleInfo> sampleInfo;
    DlsInfo info;
};

struct DlsRegion {
    DlsRegionHeader header;
    std::optional<DlsWaveSampleInfo> sampleInfo;
    std::optional<DlsWaveLink> waveLink;
    DlsArticulation articulation;
};

struct DlsInstrument {
    std::string name;
    uint32_t bank = 0;
    uint32_t program = 0;
    std::vector<DlsRegion> regions;
    DlsArticulation globalArticulation;
    DlsInfo info;
};

struct DlsFile {
    DlsPoolTable poolTable;
    std::vector<DlsInstrument> instruments;
    std::vector<DlsWave> waves;
    std::vector<uint32_t> waveOffsets;
    DlsInfo info;
    uint32_t versionMajor = 1;
    uint32_t versionMinor = 0;
};

enum class SF2GeneratorType : uint16_t {
    startAddrsOffset = 0,
    endAddrsOffset = 1,
    startloopAddrsOffset = 2,
    endloopAddrsOffset = 3,
    modLFOToPitch = 5,
    vibLFOToPitch = 6,
    modEnvToPitch = 7,
    initialFilterFc = 8,
    initialFilterQ = 9,
    modLFOToFilterFc = 10,
    modEnvToFilterFc = 11,
    modLFOToVolume = 13,
    chorusEffectsSend = 15,
    reverbEffectsSend = 16,
    pan = 17,
    delayModLFO = 21,
    freqModLFO = 22,
    delayVibLFO = 23,
    freqVibLFO = 24,
    delayModEnv = 25,
    attackModEnv = 26,
    holdModEnv = 27,
    decayModEnv = 28,
    sustainModEnv = 29,
    releaseModEnv = 30,
    keynumToModEnvHold = 31,
    keynumToModEnvDecay = 32,
    delayVolEnv = 33,
    attackVolEnv = 34,
    holdVolEnv = 35,
    decayVolEnv = 36,
    sustainVolEnv = 37,
    releaseVolEnv = 38,
    keynumToVolEnvHold = 39,
    keynumToVolEnvDecay = 40,
    instrument = 41,
    keyRange = 43,
    velRange = 44,
    initialAttenuation = 48,
    coarseTune = 51,
    fineTune = 52,
    sampleID = 53,
    sampleModes = 54,
    exclusiveClass = 57,
    overridingRootKey = 58
};

struct SF2Generator {
    SF2GeneratorType type;
    int16_t amount = 0;
};

struct SF2Modulator {
    uint16_t srcOper = 0;
    uint16_t destOper = 0;
    int16_t amount = 0;
    uint16_t amtSrcOper = 0;
    uint16_t transOper = 0;
};

struct SF2Sample {
    std::string name;
    std::vector<int16_t> sampleData;
    uint32_t sampleRate = 44100;
    uint8_t originalPitch = 60;
    int8_t pitchCorrection = 0;
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t loopStart = 0;
    uint32_t loopEnd = 0;
    uint16_t sampleLink = 0;
    uint16_t sampleType = 1;
};

struct SF2Instrument {
    std::string name;
    uint16_t bagIndex = 0;
};

struct SF2InstrumentBag {
    uint16_t genIndex = 0;
    uint16_t modIndex = 0;
};

struct SF2Preset {
    std::string name;
    uint16_t preset = 0;
    uint16_t bank = 0;
    uint16_t bagIndex = 0;
    uint32_t library = 0;
    uint32_t genre = 0;
    uint32_t morphology = 0;
};

struct SF2PresetBag {
    uint16_t genIndex = 0;
    uint16_t modIndex = 0;
};

struct SF2File {
    uint16_t ifilMajor = 2;
    uint16_t ifilMinor = 1;
    std::string isng = "EMU8000";
    std::string INAM = "Converted from DLS";
    std::string IROM;
    std::string ICRD;
    std::string IENG;
    std::string ICMT;
    std::string ICOP;
    std::string ISFT = "dls2sf2";
    std::vector<std::string> warnings;

    std::vector<SF2Sample> samples;
    std::vector<SF2Instrument> instruments;
    std::vector<SF2InstrumentBag> instrumentBags;
    std::vector<SF2Generator> instrumentGenerators;
    std::vector<SF2Modulator> instrumentModulators;
    std::vector<SF2Preset> presets;
    std::vector<SF2PresetBag> presetBags;
    std::vector<SF2Generator> presetGenerators;
    std::vector<SF2Modulator> presetModulators;
};

class DlsParser {
public:
    DlsFile parse(const std::string& filename);
    DlsFile parse(std::istream& is);
};

class ArticulationConverter {
public:
    std::vector<SF2Generator> convertGenerators(const DlsArticulation& art) const;
    std::vector<SF2Modulator> convertModulators(const DlsArticulation& art) const;
};

class Converter {
public:
    SF2File convert(const DlsFile& dls) const;
};

class SF2Writer {
public:
    void write(const std::string& filename, const SF2File& sf2);
    void write(std::ostream& os, const SF2File& sf2);
};

class Dls2SF2 {
public:
    void convert(const std::string& inputPath, const std::string& outputPath);

private:
    DlsParser parser_;
    Converter converter_;
    SF2Writer writer_;
};

}  // namespace dls2sf2
