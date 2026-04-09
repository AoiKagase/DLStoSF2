#include "dls2sf2.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>

namespace {

using namespace dls2sf2;

constexpr uint32_t makeFourCC(char a, char b, char c, char d) {
    return dls2sf2::makeFourCC(a, b, c, d);
}

void appendU16(std::string& out, uint16_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void appendS16(std::string& out, int16_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void appendU32(std::string& out, uint32_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

void appendS32(std::string& out, int32_t value) {
    out.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

std::string chunk(uint32_t id, const std::string& data) {
    std::string out;
    appendU32(out, id);
    appendU32(out, static_cast<uint32_t>(data.size()));
    out += data;
    if ((data.size() & 1U) != 0U) {
        out.push_back('\0');
    }
    return out;
}

std::string listChunk(uint32_t type, const std::string& data) {
    std::string out;
    appendU32(out, LIST_ID);
    appendU32(out, static_cast<uint32_t>(data.size() + 4));
    appendU32(out, type);
    out += data;
    if (((data.size() + 4) & 1U) != 0U) {
        out.push_back('\0');
    }
    return out;
}

std::string riffChunk(uint32_t type, const std::string& data) {
    std::string out;
    appendU32(out, RIFF_ID);
    appendU32(out, static_cast<uint32_t>(data.size() + 4));
    appendU32(out, type);
    out += data;
    if (((data.size() + 4) & 1U) != 0U) {
        out.push_back('\0');
    }
    return out;
}

std::string infoText(const char* text) {
    std::string value(text);
    value.push_back('\0');
    return value;
}

std::string defaultOutputPathForInput(const std::string& inputPath) {
    std::filesystem::path outputPath(inputPath);
    outputPath.replace_extension(".sf2");
    return outputPath.string();
}

std::string buildMinimalDls() {
    std::string infoWave = listChunk(INFO_ID, chunk(INAM_ID, infoText("WaveA")));
    std::string waveFmt;
    appendU16(waveFmt, 1);
    appendU16(waveFmt, 1);
    appendU32(waveFmt, 22050);
    appendU32(waveFmt, 44100);
    appendU16(waveFmt, 2);
    appendU16(waveFmt, 16);

    std::string waveData;
    appendS16(waveData, 0);
    appendS16(waveData, 1000);
    appendS16(waveData, -1000);
    appendS16(waveData, 0);

    std::string wsmp;
    appendU32(wsmp, 20);
    appendU16(wsmp, 60);
    appendS16(wsmp, 0);
    appendS32(wsmp, 0);
    appendU32(wsmp, 0);
    appendU32(wsmp, 1);
    appendU32(wsmp, 16);
    appendU32(wsmp, 0);
    appendU32(wsmp, 1);
    appendU32(wsmp, 2);

    std::string wtpt;
    appendU16(wtpt, 61);
    appendS16(wtpt, -3);

    std::string wave = listChunk(WAVE_ID,
                                 chunk(FMT_ID, waveFmt) + chunk(DATA_ID, waveData) +
                                     chunk(WSMP_ID, wsmp) + chunk(WTPT_ID, wtpt) + infoWave);

    std::string rgnh;
    appendU16(rgnh, 0);
    appendU16(rgnh, 127);
    appendU16(rgnh, 0);
    appendU16(rgnh, 127);
    appendU16(rgnh, 0);
    appendU16(rgnh, 3);

    std::string rgnWsmp;
    appendU32(rgnWsmp, 20);
    appendU16(rgnWsmp, 64);
    appendS16(rgnWsmp, 5);
    appendS32(rgnWsmp, 65536);
    appendU32(rgnWsmp, 0);
    appendU32(rgnWsmp, 0);

    std::string wlnk;
    appendU16(wlnk, 0);
    appendU16(wlnk, 0);
    appendU32(wlnk, 1);
    appendU32(wlnk, 0);

    std::string art1;
    appendU32(art1, 8);
    appendU32(art1, 27);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0004);
    appendU16(art1, 0);
    appendS32(art1, 32768);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0206);
    appendU16(art1, 0);
    appendS32(art1, 131072);
    appendU16(art1, 0x0002);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0001);
    appendS32(art1, -786432);
    appendU16(art1, 0x0001);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0003);
    appendU16(art1, 0x0000);
    appendS32(art1, 50 * 65536);
    appendU16(art1, 0x0001);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0000);
    appendS32(art1, 120 * 65536);
    appendU16(art1, 0x0009);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0003);
    appendU16(art1, 0x0000);
    appendS32(art1, 25 * 65536);
    appendU16(art1, 0x0005);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0003);
    appendU16(art1, 0x0000);
    appendS32(art1, 30 * 65536);
    appendU16(art1, 0x0005);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0000);
    appendS32(art1, 40 * 65536);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0005);
    appendU16(art1, 0x0000);
    appendS32(art1, 67 * 65536);
    appendU16(art1, 0x0081);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0002);
    appendS32(art1, 10 * 65536);
    appendU16(art1, 0x0081);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0003);
    appendS32(art1, 11 * 65536);
    appendU16(art1, 0x0004);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0081);
    appendU16(art1, 0x0000);
    appendS32(art1, 12 * 65536);
    appendU16(art1, 0x0002);
    appendU16(art1, 0x008B);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0002);
    appendS32(art1, 13 * 65536);
    appendU16(art1, 0x0002);
    appendU16(art1, 0x008B);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0003);
    appendS32(art1, 14 * 65536);
    appendU16(art1, 0x00DB);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0081);
    appendU16(art1, 0x0000);
    appendS32(art1, 15 * 65536);
    appendU16(art1, 0x00DD);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0080);
    appendU16(art1, 0x0000);
    appendS32(art1, 16 * 65536);
    appendU16(art1, 0x0007);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0000);
    appendS32(art1, 17 * 65536);
    appendU16(art1, 0x0008);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0500);
    appendU16(art1, 0x0000);
    appendS32(art1, 18 * 65536);
    appendU16(art1, 0x0006);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0003);
    appendU16(art1, 0x0000);
    appendS32(art1, 19 * 65536);
    appendU16(art1, 0x008A);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0004);
    appendU16(art1, 0x0000);
    appendS32(art1, 20 * 65536);
    appendU16(art1, 0x0087);
    appendU16(art1, 0x0000);
    appendU16(art1, 0x0001);
    appendU16(art1, 0x0000);
    appendS32(art1, 21 * 65536);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x7777);
    appendU16(art1, 0);
    appendS32(art1, 12345);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x020A);
    appendU16(art1, 0);
    appendS32(art1, 32768000);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0003);
    appendU16(art1, 0);
    appendS32(art1, 150 * 65536);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0001);
    appendU16(art1, 0);
    appendS32(art1, 2 * 65536);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0081);
    appendU16(art1, 0);
    appendS32(art1, 12345);
    appendU16(art1, 0);
    appendU16(art1, 0);
    appendU16(art1, 0x0010);
    appendU16(art1, 0);
    appendS32(art1, 1);

    std::string art1Extra;
    appendU32(art1Extra, 8);
    appendU32(art1Extra, 1);
    appendU16(art1Extra, 0);
    appendU16(art1Extra, 0);
    appendU16(art1Extra, 0x0004);
    appendU16(art1Extra, 0);
    appendS32(art1Extra, -25 * 65536);

    std::string region = listChunk(RGN_ID, chunk(RGNH_ID, rgnh) + chunk(WSMP_ID, rgnWsmp) +
                                               chunk(WLNK_ID, wlnk) +
                                               listChunk(LART_ID, chunk(ART1_ID, art1) + chunk(ART1_ID, art1Extra)));

    std::string insh;
    appendU32(insh, 1);
    appendU32(insh, 0x80000000U);
    appendU32(insh, 10);

    std::string infoInst = listChunk(INFO_ID, chunk(INAM_ID, infoText("InstA")));
    std::string instrument = listChunk(INS_ID, chunk(INSH_ID, insh) + listChunk(LRGN_ID, region) + infoInst);

    std::string colh;
    appendU32(colh, 1);

    std::string ptbl;
    appendU32(ptbl, 8);
    appendU32(ptbl, 1);
    appendU32(ptbl, 0);

    std::string vers;
    appendU32(vers, 1);
    appendU32(vers, 1);

    std::string infoTop = listChunk(INFO_ID, chunk(INAM_ID, infoText("TopName")));
    std::string dlsData = chunk(COLH_ID, colh) + chunk(PTBL_ID, ptbl) + chunk(VERS_ID, vers) +
                          listChunk(LINS_ID, instrument) + listChunk(WAVE_LIST_ID, wave) + infoTop;
    return riffChunk(DLS_ID, dlsData);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

uint32_t readU32At(const std::string& bytes, size_t offset) {
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

bool containsFourCC(const std::string& bytes, uint32_t fourcc) {
    for (size_t i = 0; i + 4 <= bytes.size(); ++i) {
        if (readU32At(bytes, i) == fourcc) {
            return true;
        }
    }
    return false;
}

size_t findFourCCOffset(const std::string& bytes, uint32_t fourcc, size_t occurrence = 0) {
    size_t matched = 0;
    for (size_t i = 0; i + 4 <= bytes.size(); ++i) {
        if (readU32At(bytes, i) != fourcc) {
            continue;
        }
        if (matched == occurrence) {
            return i;
        }
        ++matched;
    }
    return std::string::npos;
}

template <typename Func>
void requireThrows(Func&& fn, const std::string& expectedMessagePart) {
    try {
        fn();
    } catch (const std::exception& e) {
        if (std::string(e.what()).find(expectedMessagePart) != std::string::npos) {
            return;
        }
        throw std::runtime_error("unexpected exception text: " + std::string(e.what()));
    }
    throw std::runtime_error("expected exception containing: " + expectedMessagePart);
}

void runTests() {
    require(defaultOutputPathForInput("sound.dls") == "sound.sf2",
            "expected default output to replace extension");
    require(defaultOutputPathForInput("C:\\bank\\soundbank") == "C:\\bank\\soundbank.sf2",
            "expected default output to append sf2 extension");
    require(defaultOutputPathForInput("C:\\bank\\sound.dls") == "C:\\bank\\sound.sf2",
            "expected default output to preserve directory");

    const std::string dlsBytes = buildMinimalDls();
    std::istringstream dlsStream(dlsBytes);

    DlsParser parser;
    const DlsFile dls = parser.parse(dlsStream);
    require(dls.waves.size() == 1, "expected one wave");
    require(dls.instruments.size() == 1, "expected one instrument");
    require(dls.poolTable.cues.size() == 1, "expected pool table");
    require(dls.waves[0].sampleInfo.has_value(), "expected wave wsmp");
    require(dls.waves[0].sampleInfo->loopCount == 1, "expected stored wsmp loop count");
    require(dls.waves[0].sampleInfo->loops.size() == 1, "expected wave loop");
    require(dls.waves[0].sampleInfo->unityNote == 61, "expected wtpt root key override");
    require(dls.waves[0].sampleInfo->fineTune == -3, "expected wtpt fine tune override");
    require(dls.instruments[0].regions.size() == 1, "expected one region");
    require(dls.instruments[0].regions[0].sampleInfo.has_value(), "expected region wsmp");
    require(dls.instruments[0].regions[0].waveLink.has_value(), "expected region wlnk");
    require(dls.instruments[0].regions[0].articulation.connectionCount ==
                dls.instruments[0].regions[0].articulation.connections.size(),
            "expected merged articulation count");
    require(dls.instruments[0].regions[0].articulation.connectionCount == 28,
            "expected merged articulation chunks");
    require(!dls.instruments[0].regions[0].articulation.connections.empty(), "expected region art1");

    Converter converter;
    const SF2File sf2 = converter.convert(dls);
    require(sf2.samples.size() == 1, "expected one sample");
    require(sf2.presets.size() == 1, "expected one preset");
    require(sf2.instruments.size() == 1, "expected one sf2 instrument");
    require(sf2.presets[0].bank == 128, "expected drum bank mapping");
    require(!sf2.instrumentGenerators.empty(), "expected instrument generators");
    require(sf2.samples[0].originalPitch == 64, "expected region wsmp root key override");
    require(sf2.samples[0].pitchCorrection == 5, "expected region wsmp fine tune override");

    bool sawRootKey = false;
    bool sawAttenuation = false;
    bool sawLoopMode = false;
    bool sawExclusiveClass = false;
    bool sawPan = false;
    bool sawAttackVol = false;
    bool sawSustainVol = false;
    bool sawCoarseTune = false;
    bool sawFineTune = false;
    bool sawExtraAttenuation = false;
    bool sawReverbSend = false;
    bool sawModLfoToPitch = false;
    bool sawModLfoToFilter = false;
    bool sawVibLfoToPitch = false;
    bool sawModEnvToPitch = false;
    bool sawModEnvToFilter = false;
    bool sawRootKey67 = false;
    for (const auto& gen : sf2.instrumentGenerators) {
        if (gen.type == SF2GeneratorType::overridingRootKey && gen.amount == 64) {
            sawRootKey = true;
        }
        if (gen.type == SF2GeneratorType::initialAttenuation) {
            sawAttenuation = true;
        }
        if (gen.type == SF2GeneratorType::sampleModes && gen.amount == 1) {
            sawLoopMode = true;
        }
        if (gen.type == SF2GeneratorType::exclusiveClass && gen.amount == 3) {
            sawExclusiveClass = true;
        }
        if (gen.type == SF2GeneratorType::pan && gen.amount >= -500 && gen.amount <= 500) {
            sawPan = true;
        }
        if (gen.type == SF2GeneratorType::attackVolEnv) {
            sawAttackVol = true;
        }
        if (gen.type == SF2GeneratorType::sustainVolEnv && gen.amount > 0 && gen.amount < 1440) {
            sawSustainVol = true;
        }
        if (gen.type == SF2GeneratorType::coarseTune && gen.amount == 1) {
            sawCoarseTune = true;
        }
        if (gen.type == SF2GeneratorType::fineTune && gen.amount == 50) {
            sawFineTune = true;
        }
        if (gen.type == SF2GeneratorType::initialAttenuation && gen.amount == 2) {
            sawExtraAttenuation = true;
        }
        if (gen.type == SF2GeneratorType::reverbEffectsSend && gen.amount >= 0 && gen.amount <= 1000) {
            sawReverbSend = true;
        }
        if (gen.type == SF2GeneratorType::modLFOToPitch && gen.amount == 50) {
            sawModLfoToPitch = true;
        }
        if (gen.type == SF2GeneratorType::modLFOToFilterFc && gen.amount == 120) {
            sawModLfoToFilter = true;
        }
        if (gen.type == SF2GeneratorType::vibLFOToPitch && gen.amount == 25) {
            sawVibLfoToPitch = true;
        }
        if (gen.type == SF2GeneratorType::modEnvToPitch && gen.amount == 30) {
            sawModEnvToPitch = true;
        }
        if (gen.type == SF2GeneratorType::modEnvToFilterFc && gen.amount == 40) {
            sawModEnvToFilter = true;
        }
        if (gen.type == SF2GeneratorType::overridingRootKey && gen.amount == 67) {
            sawRootKey67 = true;
        }
    }
    require(sawRootKey, "expected overridingRootKey generator");
    require(sawAttenuation, "expected initialAttenuation generator");
    require(sawLoopMode, "expected sampleModes generator");
    require(sawExclusiveClass, "expected exclusiveClass generator");
    require(sawPan, "expected pan generator");
    require(sawAttackVol, "expected attackVolEnv generator");
    require(sawSustainVol, "expected sustainVolEnv generator");
    require(sawCoarseTune, "expected coarseTune generator");
    require(sawFineTune, "expected fineTune pitch split");
    require(sawExtraAttenuation, "expected gain/attenuation generator");
    require(sawReverbSend, "expected reverb send generator");
    require(sawModLfoToPitch, "expected modLFOToPitch generator");
    require(sawModLfoToFilter, "expected modLFOToFilterFc generator");
    require(sawVibLfoToPitch, "expected vibLFOToPitch generator");
    require(sawModEnvToPitch, "expected modEnvToPitch generator");
    require(sawModEnvToFilter, "expected modEnvToFilterFc generator");
    require(sawRootKey67, "expected keynumber to overridingRootKey generator");
    require(!sf2.instrumentModulators.empty(), "expected modulator conversion");

    bool sawVelocityToFilter = false;
    bool sawConvexCc1ToFilter = false;
    bool sawSwitchCc1ToFilter = false;
    bool sawConvexCc11AmountSource = false;
    bool sawSwitchCc11AmountSource = false;
    bool sawCc91ToReverb = false;
    bool sawCc93ToChorus = false;
    bool sawClampedReverbModAmount = false;
    bool sawPolyPressureToFilter = false;
    bool sawChannelPressureToFilter = false;
    bool sawPitchWheelToPitch = false;
    bool sawCc10ToPan = false;
    bool sawCc7ToAttenuation = false;
    for (const auto& mod : sf2.instrumentModulators) {
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) && mod.srcOper != 0 &&
            mod.amount > 0) {
            sawVelocityToFilter = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.srcOper & 0x007F) == 1 && (mod.srcOper & 0x0080) != 0 &&
            ((mod.srcOper >> 10) & 0x3F) == 2) {
            sawConvexCc1ToFilter = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.srcOper & 0x007F) == 1 && (mod.srcOper & 0x0080) != 0 &&
            ((mod.srcOper >> 10) & 0x3F) == 3) {
            sawSwitchCc1ToFilter = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.amtSrcOper & 0x007F) == 11 && (mod.amtSrcOper & 0x0080) != 0 &&
            ((mod.amtSrcOper >> 10) & 0x3F) == 2) {
            sawConvexCc11AmountSource = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.amtSrcOper & 0x007F) == 11 && (mod.amtSrcOper & 0x0080) != 0 &&
            ((mod.amtSrcOper >> 10) & 0x3F) == 3) {
            sawSwitchCc11AmountSource = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::reverbEffectsSend) &&
            (mod.srcOper & 0x007F) == 91 && (mod.srcOper & 0x0080) != 0) {
            sawCc91ToReverb = true;
            if (mod.amount == 15) {
                sawClampedReverbModAmount = true;
            }
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::chorusEffectsSend) &&
            (mod.srcOper & 0x007F) == 93 && (mod.srcOper & 0x0080) != 0) {
            sawCc93ToChorus = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.srcOper & 0x007F) == 10 && (mod.srcOper & 0x0080) == 0) {
            sawPolyPressureToFilter = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialFilterFc) &&
            (mod.srcOper & 0x007F) == 13 && (mod.srcOper & 0x0080) == 0) {
            sawChannelPressureToFilter = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::coarseTune) &&
            (mod.srcOper & 0x007F) == 14 && (mod.srcOper & 0x0080) == 0 &&
            (mod.srcOper & 0x0200) != 0) {
            sawPitchWheelToPitch = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::pan) &&
            (mod.srcOper & 0x007F) == 10 && (mod.srcOper & 0x0080) != 0) {
            sawCc10ToPan = true;
        }
        if (mod.destOper == static_cast<uint16_t>(SF2GeneratorType::initialAttenuation) &&
            (mod.srcOper & 0x007F) == 7 && (mod.srcOper & 0x0080) != 0) {
            sawCc7ToAttenuation = true;
        }
    }
    require(sawVelocityToFilter, "expected velocity-to-filter modulator");
    require(sawConvexCc1ToFilter, "expected convex CC1 filter modulator");
    require(sawSwitchCc1ToFilter, "expected switch CC1 filter modulator");
    require(sawConvexCc11AmountSource, "expected convex CC11 amount source");
    require(sawSwitchCc11AmountSource, "expected switch CC11 amount source");
    require(sawCc91ToReverb, "expected CC91 to reverb modulator");
    require(sawCc93ToChorus, "expected CC93 to chorus modulator");
    require(sawClampedReverbModAmount, "expected reverb modulator amount conversion");
    require(sawPolyPressureToFilter, "expected poly pressure to filter modulator");
    require(sawChannelPressureToFilter, "expected channel pressure to filter modulator");
    require(sawPitchWheelToPitch, "expected pitch wheel to pitch modulator");
    require(sawCc10ToPan, "expected CC10 to pan modulator");
    require(sawCc7ToAttenuation, "expected CC7 to attenuation modulator");
    require(!sf2.warnings.empty(), "expected unsupported-connection warning");

    bool sawLeftOutputWarning = false;
    bool sawUnknownWarning = false;
    bool sawFalsePositiveLfoWarning = false;
    bool sawUnrepresentableEg1Warning = false;
    bool sawLoopApproximationWarning = false;
    for (const auto& warning : sf2.warnings) {
        if (warning.find("Unrepresentable in SF2: gen dst=CONN_DST_LEFT") != std::string::npos) {
            sawLeftOutputWarning = true;
        }
        if (warning.find("0x7777") != std::string::npos) {
            sawUnknownWarning = true;
        }
        if (warning.find("CONN_SRC_LFO") != std::string::npos || warning.find("CONN_SRC_VIBRATO") != std::string::npos ||
            warning.find("CONN_SRC_EG2") != std::string::npos) {
            sawFalsePositiveLfoWarning = true;
        }
        if (warning.find("Unrepresentable in SF2: mod src=CONN_SRC_EG1") != std::string::npos) {
            sawUnrepresentableEg1Warning = true;
        }
        if (warning.find("multiple DLS loops collapsed to first loop") != std::string::npos) {
            sawLoopApproximationWarning = true;
        }
    }
    require(sawLeftOutputWarning, "expected named unrepresentable warning");
    require(sawUnknownWarning, "expected unknown destination warning");
    require(!sawFalsePositiveLfoWarning, "expected converted source generators to avoid unsupported warnings");
    require(sawUnrepresentableEg1Warning, "expected EG1 source warning to be classified as unrepresentable");
    require(!sawLoopApproximationWarning, "expected no loop approximation warning for single-loop effective wsmp");

    std::ostringstream sf2Stream(std::ios::binary);
    SF2Writer writer;
    writer.write(sf2Stream, sf2);
    const std::string sf2Bytes = sf2Stream.str();

    require(sf2Bytes.size() > 12, "expected non-empty sf2");
    require(readU32At(sf2Bytes, 0) == RIFF_ID, "expected RIFF header");
    require(readU32At(sf2Bytes, 8) == SFBK_ID, "expected sfbk");
    require(containsFourCC(sf2Bytes, INFO_ID), "expected INFO list");
    require(containsFourCC(sf2Bytes, SDTA_ID), "expected sdta list");
    require(containsFourCC(sf2Bytes, PDTA_ID), "expected pdta list");
    require(containsFourCC(sf2Bytes, SHDR_ID), "expected shdr chunk");
    require(containsFourCC(sf2Bytes, SMPL_ID), "expected smpl chunk");
    const size_t smplOffset = findFourCCOffset(sf2Bytes, SMPL_ID);
    require(smplOffset != std::string::npos, "expected smpl offset");
    require(readU32At(sf2Bytes, smplOffset + 4) == 100, "expected smpl chunk size with guard samples");
    const size_t shdrOffset = findFourCCOffset(sf2Bytes, SHDR_ID);
    require(shdrOffset != std::string::npos, "expected shdr offset");
    require(readU32At(sf2Bytes, shdrOffset + 4) == 92, "expected two 46-byte shdr records");

    DlsFile stereoDls;
    stereoDls.poolTable.cues = {0, 100};
    stereoDls.waveOffsets = {0, 100};

    DlsWave leftWave;
    leftWave.name = "Left";
    leftWave.sampleData = {0, 1, 2, 3};
    leftWave.sampleInfo = DlsWaveSampleInfo{};
    leftWave.sampleInfo->unityNote = 60;
    leftWave.sampleInfo->loopCount = 2;
    leftWave.sampleInfo->loops.push_back({16, 0, 1, 2});
    leftWave.sampleInfo->loops.push_back({16, 0, 3, 1});
    stereoDls.waves.push_back(leftWave);

    DlsWave rightWave = leftWave;
    rightWave.name = "Right";
    stereoDls.waves.push_back(rightWave);

    DlsInstrument stereoInstrument;
    stereoInstrument.name = "StereoInst";
    stereoInstrument.program = 1;

    DlsRegion leftRegion;
    leftRegion.waveLink = DlsWaveLink{};
    leftRegion.waveLink->options = 0x0002;
    leftRegion.waveLink->phaseGroup = 7;
    leftRegion.waveLink->channel = 0x00000001;
    leftRegion.waveLink->tableIndex = 0;
    stereoInstrument.regions.push_back(leftRegion);

    DlsRegion rightRegion;
    rightRegion.waveLink = DlsWaveLink{};
    rightRegion.waveLink->options = 0x0002;
    rightRegion.waveLink->phaseGroup = 7;
    rightRegion.waveLink->channel = 0x00000002;
    rightRegion.waveLink->tableIndex = 1;
    stereoInstrument.regions.push_back(rightRegion);

    stereoDls.instruments.push_back(stereoInstrument);
    const SF2File stereoSf2 = converter.convert(stereoDls);
    require(stereoSf2.samples.size() == 2, "expected stereo samples");
    require(stereoSf2.samples[0].sampleType == 4, "expected left sample type");
    require(stereoSf2.samples[1].sampleType == 2, "expected right sample type");
    require(stereoSf2.samples[0].sampleLink == 1, "expected left sample link");
    require(stereoSf2.samples[1].sampleLink == 0, "expected right sample link");

    DlsFile loneLeftDls;
    loneLeftDls.poolTable.cues = {0};
    loneLeftDls.waveOffsets = {0};
    DlsWave loneLeftWave = leftWave;
    loneLeftWave.name = "LoneLeft";
    loneLeftDls.waves.push_back(loneLeftWave);

    DlsInstrument loneLeftInstrument;
    loneLeftInstrument.name = "LoneLeftInst";
    loneLeftInstrument.program = 2;
    DlsRegion loneLeftRegion;
    loneLeftRegion.waveLink = DlsWaveLink{};
    loneLeftRegion.waveLink->options = 0x0002;
    loneLeftRegion.waveLink->phaseGroup = 9;
    loneLeftRegion.waveLink->channel = 0x00000001;
    loneLeftRegion.waveLink->tableIndex = 0;
    loneLeftInstrument.regions.push_back(loneLeftRegion);
    loneLeftDls.instruments.push_back(loneLeftInstrument);

    const SF2File loneLeftSf2 = converter.convert(loneLeftDls);
    require(loneLeftSf2.samples.size() == 1, "expected single sample");
    require(loneLeftSf2.samples[0].sampleType == 1, "expected unmatched left sample to fall back to mono");
    require(loneLeftSf2.samples[0].sampleLink == 0, "expected unmatched left sample to have no link");

    DlsFile oneShotDls;
    oneShotDls.poolTable.cues = {0};
    oneShotDls.waveOffsets = {0};
    DlsWave oneShotWave;
    oneShotWave.name = "OneShot";
    oneShotWave.sampleRate = 1000;
    oneShotWave.sampleData.resize(1000);
    oneShotDls.waves.push_back(oneShotWave);
    DlsInstrument oneShotInstrument;
    oneShotInstrument.name = "OneShotInst";
    oneShotInstrument.program = 3;
    DlsRegion oneShotRegion;
    oneShotRegion.header.options = 0x0001;
    oneShotRegion.waveLink = DlsWaveLink{};
    oneShotRegion.waveLink->tableIndex = 0;
    oneShotInstrument.regions.push_back(oneShotRegion);
    oneShotDls.instruments.push_back(oneShotInstrument);
    const SF2File oneShotSf2 = converter.convert(oneShotDls);
    bool sawOneShotRelease = false;
    for (const auto& gen : oneShotSf2.instrumentGenerators) {
        if (gen.type == SF2GeneratorType::releaseVolEnv && gen.amount == 3600) {
            sawOneShotRelease = true;
        }
    }
    require(sawOneShotRelease, "expected self-non-exclusive region to receive long release approximation");

    DlsFile explicitReleaseDls = oneShotDls;
    explicitReleaseDls.instruments[0].regions[0].articulation.connections.push_back(
        {0x0000, 0x0000, 0x0209, 0x0000, 500 * 65536});
    const SF2File explicitReleaseSf2 = converter.convert(explicitReleaseDls);
    bool sawExplicitRelease = false;
    bool sawApproximatedRelease = false;
    for (const auto& gen : explicitReleaseSf2.instrumentGenerators) {
        if (gen.type == SF2GeneratorType::releaseVolEnv && gen.amount == 500) {
            sawExplicitRelease = true;
        }
        if (gen.type == SF2GeneratorType::releaseVolEnv && gen.amount == 3600) {
            sawApproximatedRelease = true;
        }
    }
    require(sawExplicitRelease, "expected explicit release articulation to be preserved");
    require(!sawApproximatedRelease, "expected explicit release articulation to suppress one-shot approximation");

    bool stereoSawLoopApproximation = false;
    for (const auto& warning : stereoSf2.warnings) {
        if (warning.find("multiple DLS loops collapsed to first loop") != std::string::npos) {
            stereoSawLoopApproximation = true;
        }
    }
    require(stereoSawLoopApproximation, "expected multiple loop approximation warning");

    DlsFile multiChannelDls;
    DlsWave multiChannelWave;
    multiChannelWave.name = "Multi";
    multiChannelWave.channels = 2;
    multiChannelWave.sampleData = {0, 1, 2, 3};
    multiChannelDls.waves.push_back(multiChannelWave);
    const SF2File multiChannelSf2 = converter.convert(multiChannelDls);
    bool sawMultiChannelApproximation = false;
    for (const auto& warning : multiChannelSf2.warnings) {
        if (warning.find("multichannel DLS wave collapsed to first channel") != std::string::npos) {
            sawMultiChannelApproximation = true;
        }
    }
    require(sawMultiChannelApproximation, "expected multichannel wave approximation warning");

    ArticulationConverter articulationConverter;
    DlsArticulation boundaryArt;
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x0004, 0x0000, 900 * 65536});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x0081, 0x0000, 1200 * 65536});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x020A, 0x0000, 0});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x030E, 0x0000, 0});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x0005, 0x0000, 200 * 65536});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x0001, 0x0000, 2000 * 65536});
    boundaryArt.connections.push_back({0x0000, 0x0000, 0x0501, 0x0000, 2000 * 65536});
    const auto boundaryGenerators = articulationConverter.convertGenerators(boundaryArt);

    bool sawClampedPan = false;
    bool sawClampedReverb = false;
    bool sawZeroVolSustain = false;
    bool sawZeroModSustain = false;
    bool sawClampedRootKey = false;
    bool sawClampedInitialAttenuation = false;
    bool sawClampedFilterQ = false;
    for (const auto& gen : boundaryGenerators) {
        if (gen.type == SF2GeneratorType::pan && gen.amount == 500) {
            sawClampedPan = true;
        }
        if (gen.type == SF2GeneratorType::reverbEffectsSend && gen.amount == 1000) {
            sawClampedReverb = true;
        }
        if (gen.type == SF2GeneratorType::sustainVolEnv && gen.amount == 1440) {
            sawZeroVolSustain = true;
        }
        if (gen.type == SF2GeneratorType::sustainModEnv && gen.amount == 1000) {
            sawZeroModSustain = true;
        }
        if (gen.type == SF2GeneratorType::overridingRootKey && gen.amount == 127) {
            sawClampedRootKey = true;
        }
        if (gen.type == SF2GeneratorType::initialAttenuation && gen.amount == 1440) {
            sawClampedInitialAttenuation = true;
        }
        if (gen.type == SF2GeneratorType::initialFilterQ && gen.amount == 960) {
            sawClampedFilterQ = true;
        }
    }
    require(sawClampedPan, "expected pan clamp to 500");
    require(sawClampedReverb, "expected reverb clamp to 1000");
    require(sawZeroVolSustain, "expected zero volume sustain to map to 1440");
    require(sawZeroModSustain, "expected zero modulation sustain to map to 1000");
    require(sawClampedRootKey, "expected root key clamp to 127");
    require(sawClampedInitialAttenuation, "expected attenuation clamp to 1440");
    require(sawClampedFilterQ, "expected filter Q clamp to 960");

    // keynum-to-envelope mapping tests
    DlsArticulation keynumArt;
    keynumArt.connections.push_back({0x0000, 0x0000, 0x0208, 0x0000, 50 * 65536}); // EG1_KEYNUMTOHOLD
    keynumArt.connections.push_back({0x0000, 0x0000, 0x030C, 0x0000, 60 * 65536}); // EG2_KEYNUMTODECAY
    keynumArt.connections.push_back({0x0000, 0x0000, 0x0311, 0x0000, 70 * 65536}); // EG2_KEYNUMTOHOLD
    keynumArt.connections.push_back({0x0000, 0x0000, 0x0312, 0x0000, 80 * 65536}); // EG2_KEYNUMTODECAY2
    const auto keynumGenerators = articulationConverter.convertGenerators(keynumArt);

    bool sawKeynumToVolEnvHold = false;
    bool sawKeynumToModEnvDecay = false;
    bool sawKeynumToModEnvHold = false;
    bool sawKeynumToModEnvDecay2 = false;
    bool sawWrongKeynumMapping = false;
    for (const auto& gen : keynumGenerators) {
        if (gen.type == SF2GeneratorType::keynumToVolEnvHold && gen.amount == 50) {
            sawKeynumToVolEnvHold = true;
        }
        if (gen.type == SF2GeneratorType::keynumToModEnvDecay && gen.amount == 60) {
            sawKeynumToModEnvDecay = true;
        }
        if (gen.type == SF2GeneratorType::keynumToModEnvHold && gen.amount == 70) {
            sawKeynumToModEnvHold = true;
        }
        if (gen.type == SF2GeneratorType::keynumToModEnvDecay && gen.amount == 80) {
            sawKeynumToModEnvDecay2 = true;
        }
        // EG2_KEYNUMTODECAY must NOT map to VolEnvDecay (regression guard)
        if (gen.type == SF2GeneratorType::keynumToVolEnvDecay && gen.amount == 60) {
            sawWrongKeynumMapping = true;
        }
    }
    require(sawKeynumToVolEnvHold, "expected EG1_KEYNUMTOHOLD -> keynumToVolEnvHold");
    require(sawKeynumToModEnvDecay, "expected EG2_KEYNUMTODECAY -> keynumToModEnvDecay");
    require(sawKeynumToModEnvHold, "expected EG2_KEYNUMTOHOLD -> keynumToModEnvHold");
    require(sawKeynumToModEnvDecay2, "expected EG2_KEYNUMTODECAY2 -> keynumToModEnvDecay");
    require(!sawWrongKeynumMapping, "EG2_KEYNUMTODECAY must not map to keynumToVolEnvDecay");

    std::string invalidColhDls = dlsBytes;
    const size_t colhOffset = findFourCCOffset(invalidColhDls, COLH_ID);
    require(colhOffset != std::string::npos, "expected colh chunk in synthetic dls");
    const uint32_t invalidInstrumentCount = 2;
    std::memcpy(invalidColhDls.data() + colhOffset + 8, &invalidInstrumentCount, sizeof(invalidInstrumentCount));
    requireThrows([&]() {
        std::istringstream invalidColhStream(invalidColhDls);
        parser.parse(invalidColhStream);
    }, "colh instrument count");

    std::string invalidInshDls = dlsBytes;
    const size_t inshOffset = findFourCCOffset(invalidInshDls, INSH_ID);
    require(inshOffset != std::string::npos, "expected insh chunk in synthetic dls");
    const uint32_t invalidRegionCount = 2;
    std::memcpy(invalidInshDls.data() + inshOffset + 8, &invalidRegionCount, sizeof(invalidRegionCount));
    requireThrows([&]() {
        std::istringstream invalidInshStream(invalidInshDls);
        parser.parse(invalidInshStream);
    }, "insh region count");

    std::string invalidPtblDls = dlsBytes;
    const size_t ptblCueOffset = findFourCCOffset(invalidPtblDls, PTBL_ID);
    require(ptblCueOffset != std::string::npos, "expected ptbl chunk in synthetic dls");
    const uint32_t invalidCueCount = 2;
    std::memcpy(invalidPtblDls.data() + ptblCueOffset + 12, &invalidCueCount, sizeof(invalidCueCount));
    requireThrows([&]() {
        std::istringstream invalidPtblStream(invalidPtblDls);
        parser.parse(invalidPtblStream);
    }, "declares more cues than fit in chunk");

    std::string invalidWsmpDls = dlsBytes;
    const size_t wsmpOffset = findFourCCOffset(invalidWsmpDls, WSMP_ID);
    require(wsmpOffset != std::string::npos, "expected wsmp chunk in synthetic dls");
    const uint32_t invalidLoopCount = 2;
    std::memcpy(invalidWsmpDls.data() + wsmpOffset + 24, &invalidLoopCount, sizeof(invalidLoopCount));
    requireThrows([&]() {
        std::istringstream invalidWsmpStream(invalidWsmpDls);
        parser.parse(invalidWsmpStream);
    }, "wsmp loop count exceeds chunk size");

    std::string invalidArtDls = dlsBytes;
    const size_t artOffset = findFourCCOffset(invalidArtDls, ART1_ID);
    require(artOffset != std::string::npos, "expected art1 chunk in synthetic dls");
    const uint32_t invalidConnectionCount = 64;
    std::memcpy(invalidArtDls.data() + artOffset + 12, &invalidConnectionCount, sizeof(invalidConnectionCount));
    requireThrows([&]() {
        std::istringstream invalidArtStream(invalidArtDls);
        parser.parse(invalidArtStream);
    }, "articulation connection count exceeds chunk size");

    std::string truncatedDls = dlsBytes.substr(0, dlsBytes.size() - 3);
    requireThrows([&]() {
        std::istringstream truncatedStream(truncatedDls);
        parser.parse(truncatedStream);
    }, "Unexpected EOF");
}

void runSampleFileSmokeTest(const std::string& inputPath, const std::string& outputPath) {
    if (!std::filesystem::exists(inputPath)) {
        return;
    }

    DlsParser parser;
    Converter converter;
    SF2Writer writer;
    const DlsFile dls = parser.parse(inputPath);
    const SF2File sf2 = converter.convert(dls);
    writer.write(outputPath, sf2);

    if (!sf2.warnings.empty()) {
        std::ostringstream os;
        os << "Conversion warnings for sample file:\n";
        for (const auto& warning : sf2.warnings) {
            os << warning << "\n";
        }
        throw std::runtime_error(os.str());
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (argc == 2) {
            Dls2SF2 app;
            app.convert(argv[1], defaultOutputPathForInput(argv[1]));
            return 0;
        }

        if (argc == 3) {
            Dls2SF2 app;
            app.convert(argv[1], argv[2]);
            return 0;
        }

        runTests();
        runSampleFileSmokeTest("H:\\sourcecode\\X-Ark_MIDI_ENGINE\\output\\Release\\ExtractedSoundbank_01.dls",
                               "H:\\sourcecode\\DLS2SF2\\build\\sample_smoke.sf2");
        std::cout << "All tests passed." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
