#include "dls2sf2_internal.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace dls2sf2::internal {

uint32_t readU32(std::istream& is) {
    uint32_t value = 0;
    is.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading uint32");
    }
    return value;
}

uint16_t readU16(std::istream& is) {
    uint16_t value = 0;
    is.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading uint16");
    }
    return value;
}

int16_t readS16(std::istream& is) {
    int16_t value = 0;
    is.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading int16");
    }
    return value;
}

int32_t readS32(std::istream& is) {
    int32_t value = 0;
    is.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading int32");
    }
    return value;
}

uint8_t readU8(std::istream& is) {
    uint8_t value = 0;
    is.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading uint8");
    }
    return value;
}

std::string readBytes(std::istream& is, size_t size) {
    std::string value(size, '\0');
    is.read(value.data(), static_cast<std::streamsize>(size));
    if (!is) {
        throw std::runtime_error("Unexpected EOF while reading byte block");
    }
    return value;
}

std::string trimNullTerminated(const std::string& value) {
    const size_t pos = value.find('\0');
    return pos == std::string::npos ? value : value.substr(0, pos);
}

void skipPadding(std::istream& is, uint32_t size) {
    if ((size & 1U) != 0U) {
        is.seekg(1, std::ios::cur);
    }
}

void seekToChunkEnd(std::istream& is, const RiffChunk& chunk) {
    is.seekg(chunk.dataStart + static_cast<std::streamoff>(chunk.size), std::ios::beg);
    skipPadding(is, chunk.size);
}

RiffChunk readChunk(std::istream& is) {
    RiffChunk chunk;
    chunk.id = readU32(is);
    chunk.size = readU32(is);
    chunk.dataStart = is.tellg();
    if (chunk.id == RIFF_ID || chunk.id == LIST_ID) {
        if (chunk.size < 4) {
            throw std::runtime_error("RIFF/LIST chunk size smaller than type field");
        }
        chunk.type = readU32(is);
        chunk.dataStart = is.tellg();
        chunk.size -= 4;
    }
    return chunk;
}

std::streampos chunkEnd(const RiffChunk& chunk) {
    return chunk.dataStart + static_cast<std::streamoff>(chunk.size);
}

std::streampos chunkHeaderStart(const RiffChunk& chunk) {
    return chunk.dataStart - static_cast<std::streamoff>(chunk.type ? 12 : 8);
}

void writeU32(std::ostream& os, uint32_t value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeU16(std::ostream& os, uint16_t value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeFixedName(char* dst, const std::string& src) {
    std::memset(dst, 0, 20);
    const size_t copyLen = std::min<size_t>(19, src.size());
    std::memcpy(dst, src.data(), copyLen);
}

ChunkWriter::ChunkWriter(std::ostream& os) : os_(os), sizePos_(os.tellp()) {}

void ChunkWriter::beginChunk(uint32_t id) {
    writeU32(os_, id);
    sizePos_ = os_.tellp();
    writeU32(os_, 0);
}

void ChunkWriter::beginList(uint32_t type) {
    beginChunk(LIST_ID);
    writeU32(os_, type);
}

void ChunkWriter::end() {
    const std::streampos endPos = os_.tellp();
    const uint32_t size = static_cast<uint32_t>(endPos - sizePos_ - std::streamoff(4));
    os_.seekp(sizePos_);
    writeU32(os_, size);
    os_.seekp(endPos);
    if ((size & 1U) != 0U) {
        os_.put('\0');
    }
}

int16_t clampI16(int32_t value) {
    return static_cast<int16_t>(std::clamp<int32_t>(
        value, std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()));
}

int8_t clampI8(int32_t value) {
    return static_cast<int8_t>(
        std::clamp<int32_t>(value, std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max()));
}

uint16_t makeRangeAmount(uint16_t low, uint16_t high) {
    return static_cast<uint16_t>((high << 8) | low);
}

uint16_t sf2SampleTypeFromLink(const DlsWaveLink* link) {
    if (link == nullptr) {
        return 1;
    }

    const bool isMultiChannel = (link->options & DLS_WAVELINK_OPTION_MULTICHANNEL) != 0U;
    const bool isPhaseMaster = (link->options & DLS_WAVELINK_OPTION_PHASE_MASTER) != 0U;
    const bool isLeft = (link->channel & DLS_CHANNEL_LEFT) != 0U;
    const bool isRight = (link->channel & DLS_CHANNEL_RIGHT) != 0U;

    if (isMultiChannel || isPhaseMaster || isLeft || isRight) {
        if (isLeft && !isRight) {
            return 4;
        }
        if (isRight && !isLeft) {
            return 2;
        }
        return 8;
    }
    return 1;
}

const DlsWaveSampleInfo* effectiveWsmp(const DlsRegion& region, const DlsWave& wave) {
    return region.sampleInfo ? &*region.sampleInfo : (wave.sampleInfo ? &*wave.sampleInfo : nullptr);
}

DlsWaveSampleInfo mergedWsmp(const DlsRegion& region, const DlsWave& wave) {
    DlsWaveSampleInfo merged;
    bool hasValue = false;

    if (wave.sampleInfo) {
        merged = *wave.sampleInfo;
        hasValue = true;
    }
    if (region.sampleInfo) {
        if (!hasValue) {
            return *region.sampleInfo;
        }
        merged.unityNote = region.sampleInfo->unityNote;
        merged.fineTune = region.sampleInfo->fineTune;
        merged.attenuation = region.sampleInfo->attenuation;
        merged.options = region.sampleInfo->options;
        if (!region.sampleInfo->loops.empty()) {
            merged.loops = region.sampleInfo->loops;
        }
    }

    return merged;
}

std::string fallbackName(const std::string& preferred, const std::string& prefix, size_t index) {
    return preferred.empty() ? (prefix + " " + std::to_string(index)) : preferred;
}

int16_t dlsFixedToSf2Timecents(int32_t value) {
    return clampI16(value / 65536);
}

int16_t dlsFixedToSf2Cents(int32_t value) {
    return clampI16(value / 65536);
}

int16_t dlsFixedPercentToSf2ModSustain(int32_t value) {
    const int32_t absoluteTenthsPercent = std::clamp<int32_t>(value / 65536, 0, 1000);
    return clampI16(1000 - absoluteTenthsPercent);
}

int16_t dlsFixedPercentToSf2VolSustain(int32_t value) {
    const double absoluteTenthsPercent = std::clamp<double>(static_cast<double>(value) / 65536.0, 0.0, 1000.0);
    if (absoluteTenthsPercent <= 0.0) {
        return 1440;
    }
    const double percent = absoluteTenthsPercent / 10.0;
    const double attenuationCb = -200.0 * std::log10(percent / 100.0);
    return clampI16(static_cast<int32_t>(attenuationCb + 0.5));
}

int16_t scaleGeneratorAmount(const DlsConnection& connection, SF2GeneratorType type) {
    if (connection.destination == DLS_DST_ATTENUATION || connection.destination == DLS2_DST_GAIN) {
        return clampI16(std::clamp<int32_t>(std::abs(connection.scale) / 65536, 0, 1440));
    }

    if (type == SF2GeneratorType::overridingRootKey) {
        return clampI16(std::clamp<int32_t>(connection.scale / 65536, 0, 127));
    }

    switch (connection.destination) {
        case DLS_DST_PITCH:
            if (type == SF2GeneratorType::coarseTune) {
                return clampI16(connection.scale / (100 * 65536));
            }
            return dlsFixedToSf2Cents(connection.scale);
        case DLS_DST_FILTER_CUTOFF:
            return dlsFixedToSf2Cents(connection.scale);
        case DLS_DST_FILTER_Q:
            return clampI16(std::clamp<int32_t>(connection.scale / 65536, 0, 960));
        case DLS_DST_LFO_FREQUENCY:
        case DLS_DST_LFO_STARTDELAY:
        case DLS_DST_VIB_FREQUENCY:
        case DLS_DST_VIB_STARTDELAY:
        case DLS_DST_EG1_ATTACKTIME:
        case DLS_DST_EG1_DECAYTIME:
        case DLS_DST_EG1_RELEASETIME:
        case DLS_DST_EG1_DELAYTIME:
        case DLS_DST_EG1_HOLDTIME:
        case DLS_DST_EG2_ATTACKTIME:
        case DLS_DST_EG2_DECAYTIME:
        case DLS_DST_EG2_RELEASETIME:
        case DLS_DST_EG2_DELAYTIME:
        case DLS_DST_EG2_HOLDTIME:
            return dlsFixedToSf2Timecents(connection.scale);
        case DLS_DST_EG1_SUSTAINLEVEL:
            return dlsFixedPercentToSf2VolSustain(connection.scale);
        case DLS_DST_EG2_SUSTAINLEVEL:
            return dlsFixedPercentToSf2ModSustain(connection.scale);
        case DLS2_DST_CHORUS:
        case DLS2_DST_REVERB:
            return clampI16(std::clamp<int32_t>(connection.scale / 65536, 0, 1000));
        default:
            if (type == SF2GeneratorType::pan) {
                return clampI16(std::clamp<int32_t>(connection.scale / 65536, -500, 500));
            }
            return clampI16(connection.scale / 65536);
    }
}

std::vector<SF2Generator> convertSourceConnectionGenerators(const DlsConnection& connection) {
    std::vector<SF2Generator> result;
    if (connection.control != DLS_SRC_NONE || connection.transform != DLS_TRN_NONE) {
        return result;
    }

    auto addGenerator = [&](SF2GeneratorType type) {
        result.push_back({type, scaleGeneratorAmount(connection, type)});
    };

    switch (connection.source) {
        case DLS_SRC_LFO:
            switch (connection.destination) {
                case DLS_DST_PITCH:
                    addGenerator(SF2GeneratorType::modLFOToPitch);
                    break;
                case DLS_DST_FILTER_CUTOFF:
                    addGenerator(SF2GeneratorType::modLFOToFilterFc);
                    break;
                case DLS_DST_ATTENUATION:
                    addGenerator(SF2GeneratorType::modLFOToVolume);
                    break;
                default:
                    break;
            }
            break;
        case DLS_SRC_EG2:
            switch (connection.destination) {
                case DLS_DST_PITCH:
                    addGenerator(SF2GeneratorType::modEnvToPitch);
                    break;
                case DLS_DST_FILTER_CUTOFF:
                    addGenerator(SF2GeneratorType::modEnvToFilterFc);
                    break;
                default:
                    break;
            }
            break;
        case DLS2_SRC_VIBRATO:
            if (connection.destination == DLS_DST_PITCH) {
                addGenerator(SF2GeneratorType::vibLFOToPitch);
            }
            break;
        default:
            break;
    }

    return result;
}

std::optional<SF2Generator> mapSimpleConnection(const DlsConnection& connection) {
    if (connection.source != DLS_SRC_NONE || connection.control != DLS_SRC_NONE || connection.transform != DLS_TRN_NONE) {
        return std::nullopt;
    }

    SF2Generator gen;
    bool mapped = false;
    switch (connection.destination) {
        case DLS_DST_ATTENUATION:
            gen.type = SF2GeneratorType::initialAttenuation;
            mapped = true;
            break;
        case DLS_DST_PITCH:
            gen.type = SF2GeneratorType::coarseTune;
            mapped = true;
            break;
        case DLS_DST_PAN:
            gen.type = SF2GeneratorType::pan;
            mapped = true;
            break;
        case DLS2_DST_KEYNUMBER:
            gen.type = SF2GeneratorType::overridingRootKey;
            mapped = true;
            break;
        case DLS2_DST_CHORUS:
            gen.type = SF2GeneratorType::chorusEffectsSend;
            mapped = true;
            break;
        case DLS2_DST_REVERB:
            gen.type = SF2GeneratorType::reverbEffectsSend;
            mapped = true;
            break;
        case DLS_DST_LFO_FREQUENCY:
            gen.type = SF2GeneratorType::freqModLFO;
            mapped = true;
            break;
        case DLS_DST_LFO_STARTDELAY:
            gen.type = SF2GeneratorType::delayModLFO;
            mapped = true;
            break;
        case DLS_DST_VIB_FREQUENCY:
            gen.type = SF2GeneratorType::freqVibLFO;
            mapped = true;
            break;
        case DLS_DST_VIB_STARTDELAY:
            gen.type = SF2GeneratorType::delayVibLFO;
            mapped = true;
            break;
        case DLS_DST_EG1_ATTACKTIME:
            gen.type = SF2GeneratorType::attackVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG1_DECAYTIME:
            gen.type = SF2GeneratorType::decayVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG1_RELEASETIME:
            gen.type = SF2GeneratorType::releaseVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG1_SUSTAINLEVEL:
            gen.type = SF2GeneratorType::sustainVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG1_DELAYTIME:
            gen.type = SF2GeneratorType::delayVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG1_HOLDTIME:
            gen.type = SF2GeneratorType::holdVolEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_ATTACKTIME:
            gen.type = SF2GeneratorType::attackModEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_DECAYTIME:
            gen.type = SF2GeneratorType::decayModEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_RELEASETIME:
            gen.type = SF2GeneratorType::releaseModEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_SUSTAINLEVEL:
            gen.type = SF2GeneratorType::sustainModEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_DELAYTIME:
            gen.type = SF2GeneratorType::delayModEnv;
            mapped = true;
            break;
        case DLS_DST_EG2_HOLDTIME:
            gen.type = SF2GeneratorType::holdModEnv;
            mapped = true;
            break;
        case DLS_DST_FILTER_CUTOFF:
            gen.type = SF2GeneratorType::initialFilterFc;
            mapped = true;
            break;
        case DLS_DST_FILTER_Q:
            gen.type = SF2GeneratorType::initialFilterQ;
            mapped = true;
            break;
        case DLS_DST_EG1_KEYNUMTOHOLD:
            gen.type = SF2GeneratorType::keynumToVolEnvHold;
            mapped = true;
            break;
        case DLS_DST_EG2_KEYNUMTODECAY:
            gen.type = SF2GeneratorType::keynumToModEnvDecay;
            mapped = true;
            break;
        case DLS_DST_EG2_KEYNUMTOHOLD:
            gen.type = SF2GeneratorType::keynumToModEnvHold;
            mapped = true;
            break;
        case DLS_DST_EG2_KEYNUMTODECAY2:
            gen.type = SF2GeneratorType::keynumToModEnvDecay;
            mapped = true;
            break;
        default:
            break;
    }

    if (!mapped) {
        return std::nullopt;
    }
    gen.amount = scaleGeneratorAmount(connection, gen.type);
    return gen;
}

std::optional<uint16_t> mapDestinationToGenerator(uint16_t destination) {
    DlsConnection constantConnection;
    constantConnection.destination = destination;
    const auto gen = mapSimpleConnection(constantConnection);
    if (!gen) {
        return std::nullopt;
    }
    return static_cast<uint16_t>(gen->type);
}

static uint16_t encodeSf2Source(uint16_t index, bool isCc, bool negative, bool bipolar, uint16_t type) {
    return static_cast<uint16_t>((index & 0x7FU) | (isCc ? 0x0080U : 0U) | (negative ? 0x0100U : 0U) |
                                 (bipolar ? 0x0200U : 0U) | ((type & 0x3FU) << 10U));
}

static uint16_t sf2SourceTypeFromTransform(uint16_t transform) {
    switch (transform) {
        case DLS_TRN_CONCAVE:
            return 1U;
        case DLS2_TRN_CONVEX:
            return 2U;
        case DLS2_TRN_SWITCH:
            return 3U;
        default:
            return 0U;
    }
}

std::optional<uint16_t> mapDlsControlSource(uint16_t source, uint16_t transform, bool negativeFromScale) {
    const uint16_t sourceType = sf2SourceTypeFromTransform(transform);
    switch (source) {
        case DLS_SRC_NONE:
            return 0U;
        case DLS_SRC_KEYONVELOCITY:
            return encodeSf2Source(2, false, negativeFromScale, false, sourceType);
        case DLS_SRC_KEYNUMBER:
            return encodeSf2Source(3, false, negativeFromScale, false, sourceType);
        case DLS_SRC_PITCHWHEEL:
            return encodeSf2Source(14, false, negativeFromScale, true, 0);
        case DLS2_SRC_POLYPRESSURE:
            return encodeSf2Source(10, false, negativeFromScale, false, sourceType);
        case DLS2_SRC_CHANNELPRESSURE:
        case DLS2_SRC_MONOPRESSURE:
            return encodeSf2Source(13, false, negativeFromScale, false, sourceType);
        case DLS_SRC_CC1:
            return encodeSf2Source(1, true, negativeFromScale, false, sourceType);
        case DLS_SRC_CC7:
            return encodeSf2Source(7, true, negativeFromScale, false, sourceType);
        case DLS_SRC_CC10:
            return encodeSf2Source(10, true, negativeFromScale, false, sourceType);
        case DLS_SRC_CC11:
            return encodeSf2Source(11, true, negativeFromScale, false, sourceType);
        case DLS2_SRC_CC91:
            return encodeSf2Source(91, true, negativeFromScale, false, sourceType);
        case DLS2_SRC_CC93:
            return encodeSf2Source(93, true, negativeFromScale, false, sourceType);
        case DLS_SRC_RPN0:
            return encodeSf2Source(16, false, negativeFromScale, false, 0);
        default:
            return std::nullopt;
    }
}

std::optional<SF2Modulator> mapConnectionToModulator(const DlsConnection& connection) {
    if (connection.source == DLS_SRC_NONE && connection.control == DLS_SRC_NONE) {
        return std::nullopt;
    }

    const auto destination = mapDestinationToGenerator(connection.destination);
    if (!destination) {
        return std::nullopt;
    }

    const bool negative = connection.scale < 0;
    const auto primary = mapDlsControlSource(connection.source, connection.transform, negative);
    if (!primary) {
        return std::nullopt;
    }

    const auto amountSource = mapDlsControlSource(connection.control, connection.transform, false);
    if (!amountSource) {
        return std::nullopt;
    }

    SF2Modulator mod;
    mod.srcOper = *primary;
    mod.destOper = *destination;
    mod.amount = clampI16(std::abs(scaleGeneratorAmount(connection, static_cast<SF2GeneratorType>(*destination))));
    mod.amtSrcOper = *amountSource;
    mod.transOper = 0;
    return mod;
}

std::vector<SF2Generator> convertConstantConnectionGenerators(const DlsConnection& connection) {
    std::vector<SF2Generator> result;
    if (connection.source != DLS_SRC_NONE) {
        return convertSourceConnectionGenerators(connection);
    }

    if (connection.control != DLS_SRC_NONE || connection.transform != DLS_TRN_NONE) {
        return result;
    }

    if (connection.destination == DLS_DST_PITCH) {
        const int32_t cents = dlsFixedToSf2Cents(connection.scale);
        const int32_t coarse = cents / 100;
        const int32_t fine = cents - (coarse * 100);
        if (coarse != 0) {
            result.push_back({SF2GeneratorType::coarseTune, clampI16(coarse)});
        }
        if (fine != 0 || result.empty()) {
            result.push_back({SF2GeneratorType::fineTune, clampI16(fine)});
        }
        return result;
    }

    const auto gen = mapSimpleConnection(connection);
    if (gen) {
        result.push_back(*gen);
    }
    return result;
}

std::vector<int16_t> buildSmplData(std::vector<SF2Sample>& samples) {
    std::vector<int16_t> smpl;
    for (auto& sample : samples) {
        sample.start = static_cast<uint32_t>(smpl.size());
        smpl.insert(smpl.end(), sample.sampleData.begin(), sample.sampleData.end());
        sample.end = static_cast<uint32_t>(smpl.size());

        if (sample.loopEnd > sample.loopStart && sample.loopEnd <= sample.sampleData.size()) {
            sample.loopStart += sample.start;
            sample.loopEnd += sample.start;
        } else {
            sample.loopStart = sample.end;
            sample.loopEnd = sample.end;
        }

        smpl.insert(smpl.end(), 46, 0);
    }
    return smpl;
}

std::string dlsSourceName(uint16_t value) {
    switch (value) {
        case DLS_SRC_NONE:
            return "CONN_SRC_NONE";
        case DLS_SRC_LFO:
            return "CONN_SRC_LFO";
        case DLS_SRC_KEYONVELOCITY:
            return "CONN_SRC_KEYONVELOCITY";
        case DLS_SRC_KEYNUMBER:
            return "CONN_SRC_KEYNUMBER";
        case DLS_SRC_EG1:
            return "CONN_SRC_EG1";
        case DLS_SRC_EG2:
            return "CONN_SRC_EG2";
        case DLS_SRC_PITCHWHEEL:
            return "CONN_SRC_PITCHWHEEL";
        case DLS2_SRC_POLYPRESSURE:
            return "CONN_SRC_POLYPRESSURE";
        case DLS2_SRC_CHANNELPRESSURE:
            return "CONN_SRC_CHANNELPRESSURE";
        case DLS2_SRC_VIBRATO:
            return "CONN_SRC_VIBRATO";
        case DLS2_SRC_MONOPRESSURE:
            return "CONN_SRC_MONOPRESSURE";
        case DLS_SRC_CC1:
            return "CONN_SRC_CC1";
        case DLS_SRC_CC7:
            return "CONN_SRC_CC7";
        case DLS_SRC_CC10:
            return "CONN_SRC_CC10";
        case DLS_SRC_CC11:
            return "CONN_SRC_CC11";
        case DLS2_SRC_CC91:
            return "CONN_SRC_CC91";
        case DLS2_SRC_CC93:
            return "CONN_SRC_CC93";
        case DLS_SRC_RPN0:
            return "CONN_SRC_RPN0";
        case DLS_SRC_RPN1:
            return "CONN_SRC_RPN1";
        case DLS_SRC_RPN2:
            return "CONN_SRC_RPN2";
        default: {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "0x%04X", value);
            return buffer;
        }
    }
}

std::string dlsDestinationName(uint16_t value) {
    if (value == DLS2_DST_GAIN) {
        return "CONN_DST_GAIN";
    }
    switch (value) {
        case DLS_DST_NONE:
            return "CONN_DST_NONE";
        case DLS_DST_ATTENUATION:
            return "CONN_DST_ATTENUATION";
        case DLS_DST_PITCH:
            return "CONN_DST_PITCH";
        case DLS_DST_PAN:
            return "CONN_DST_PAN";
        case DLS2_DST_KEYNUMBER:
            return "CONN_DST_KEYNUMBER";
        case DLS2_DST_LEFT:
            return "CONN_DST_LEFT";
        case DLS2_DST_RIGHT:
            return "CONN_DST_RIGHT";
        case DLS2_DST_CENTER:
            return "CONN_DST_CENTER";
        case DLS2_DST_LEFTREAR:
            return "CONN_DST_LEFTREAR";
        case DLS2_DST_RIGHTREAR:
            return "CONN_DST_RIGHTREAR";
        case DLS2_DST_LFE_CHANNEL:
            return "CONN_DST_LFE_CHANNEL";
        case DLS2_DST_CHORUS:
            return "CONN_DST_CHORUS";
        case DLS2_DST_REVERB:
            return "CONN_DST_REVERB";
        case DLS_DST_LFO_FREQUENCY:
            return "CONN_DST_LFO_FREQUENCY";
        case DLS_DST_LFO_STARTDELAY:
            return "CONN_DST_LFO_STARTDELAY";
        case DLS_DST_VIB_FREQUENCY:
            return "CONN_DST_VIB_FREQUENCY";
        case DLS_DST_VIB_STARTDELAY:
            return "CONN_DST_VIB_STARTDELAY";
        case DLS_DST_EG1_ATTACKTIME:
            return "CONN_DST_EG1_ATTACKTIME";
        case DLS_DST_EG1_DECAYTIME:
            return "CONN_DST_EG1_DECAYTIME";
        case DLS_DST_EG1_RELEASETIME:
            return "CONN_DST_EG1_RELEASETIME";
        case DLS_DST_EG1_SUSTAINLEVEL:
            return "CONN_DST_EG1_SUSTAINLEVEL";
        case DLS_DST_EG1_DELAYTIME:
            return "CONN_DST_EG1_DELAYTIME";
        case DLS_DST_EG1_HOLDTIME:
            return "CONN_DST_EG1_HOLDTIME";
        case DLS_DST_EG1_KEYNUMTOHOLD:
            return "CONN_DST_KEYNUMTOVOLENVHOLD";
        case DLS_DST_EG2_ATTACKTIME:
            return "CONN_DST_EG2_ATTACKTIME";
        case DLS_DST_EG2_DECAYTIME:
            return "CONN_DST_EG2_DECAYTIME";
        case DLS_DST_EG2_RELEASETIME:
            return "CONN_DST_EG2_RELEASETIME";
        case DLS_DST_EG2_SUSTAINLEVEL:
            return "CONN_DST_EG2_SUSTAINLEVEL";
        case DLS_DST_EG2_DELAYTIME:
            return "CONN_DST_EG2_DELAYTIME";
        case DLS_DST_EG2_HOLDTIME:
            return "CONN_DST_EG2_HOLDTIME";
        case DLS_DST_EG2_KEYNUMTODECAY:
            return "CONN_DST_KEYNUMTOVOLENVDECAY";
        case DLS_DST_EG2_KEYNUMTOHOLD:
            return "CONN_DST_KEYNUMTOMODENVHOLD";
        case DLS_DST_EG2_KEYNUMTODECAY2:
            return "CONN_DST_KEYNUMTOMODENVDECAY";
        case DLS_DST_FILTER_CUTOFF:
            return "CONN_DST_FILTER_CUTOFF";
        case DLS_DST_FILTER_Q:
            return "CONN_DST_FILTER_Q";
        default: {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "0x%04X", value);
            return buffer;
        }
    }
}

std::string dlsTransformName(uint16_t value) {
    switch (value) {
        case DLS_TRN_NONE:
            return "CONN_TRN_NONE";
        case DLS_TRN_CONCAVE:
            return "CONN_TRN_CONCAVE";
        case DLS2_TRN_CONVEX:
            return "CONN_TRN_CONVEX";
        case DLS2_TRN_SWITCH:
            return "CONN_TRN_SWITCH";
        default: {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "0x%04X", value);
            return buffer;
        }
    }
}

}  // namespace dls2sf2::internal
