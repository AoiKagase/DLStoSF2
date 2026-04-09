#include "dls2sf2_internal.hpp"

#include <algorithm>

namespace dls2sf2 {
namespace {

void parseInfoChunk(std::istream& is, uint32_t size, DlsInfo& info, uint32_t chunkId) {
    const std::string text = internal::trimNullTerminated(internal::readBytes(is, size));
    switch (chunkId) {
        case INAM_ID:
            info.name = text;
            break;
        case ICRD_ID:
            info.creationDate = text;
            break;
        case IENG_ID:
            info.engineer = text;
            break;
        case ICMT_ID:
            info.comment = text;
            break;
        case ICOP_ID:
            info.copyright = text;
            break;
        default:
            break;
    }
}

void parseInfoList(std::istream& is, const internal::RiffChunk& listChunk, DlsInfo& info) {
    while (is.tellg() < internal::chunkEnd(listChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == RIFF_ID || child.id == LIST_ID) {
            internal::seekToChunkEnd(is, child);
            continue;
        }
        parseInfoChunk(is, child.size, info, child.id);
        internal::seekToChunkEnd(is, child);
    }
}

void mergeArticulation(DlsArticulation& dst, const DlsArticulation& src) {
    if (src.connections.empty()) {
        return;
    }
    if (dst.cbSize < src.cbSize) {
        dst.cbSize = src.cbSize;
    }
    dst.connectionCount += src.connectionCount;
    dst.connections.insert(dst.connections.end(), src.connections.begin(), src.connections.end());
}

std::optional<DlsWaveSampleInfo> parseWsmp(std::istream& is, uint32_t size) {
    if (size < 20) {
        is.seekg(size, std::ios::cur);
        return std::nullopt;
    }

    DlsWaveSampleInfo wsmp;
    wsmp.cbSize = internal::readU32(is);
    wsmp.unityNote = internal::readU16(is);
    wsmp.fineTune = internal::readS16(is);
    wsmp.attenuation = internal::readS32(is);
    wsmp.options = internal::readU32(is);
    wsmp.loopCount = internal::readU32(is);

    if (wsmp.cbSize < 20 || wsmp.cbSize > size) {
        throw std::runtime_error("DLS wsmp header size exceeds chunk size");
    }

    if (wsmp.cbSize > 20 && wsmp.cbSize <= size) {
        is.seekg(static_cast<std::streamoff>(wsmp.cbSize - 20), std::ios::cur);
    }

    const uint32_t availableLoopBytes = size - wsmp.cbSize;
    if (wsmp.loopCount > (availableLoopBytes / 16U)) {
        throw std::runtime_error("DLS wsmp loop count exceeds chunk size");
    }

    const std::streampos endPos = is.tellg() + static_cast<std::streamoff>(size - wsmp.cbSize);
    for (uint32_t i = 0; i < wsmp.loopCount && is.tellg() + std::streamoff(16) <= endPos; ++i) {
        DlsWaveLoop loop;
        loop.cbSize = internal::readU32(is);
        if (loop.cbSize < 16) {
            throw std::runtime_error("DLS wsmp loop header size is smaller than WLOOP");
        }
        loop.loopType = internal::readU32(is);
        loop.loopStart = internal::readU32(is);
        loop.loopLength = internal::readU32(is);
        wsmp.loops.push_back(loop);
        if (loop.cbSize > 16) {
            const std::streamoff extraLoopBytes = static_cast<std::streamoff>(loop.cbSize - 16);
            if (is.tellg() + extraLoopBytes > endPos) {
                throw std::runtime_error("DLS wsmp loop header size exceeds chunk size");
            }
            is.seekg(extraLoopBytes, std::ios::cur);
        }
    }

    if (is.tellg() < endPos) {
        is.seekg(endPos, std::ios::beg);
    }
    return wsmp;
}

DlsWaveSampleInfo parseWtpt(std::istream& is, uint32_t size) {
    DlsWaveSampleInfo wtpt;
    if (size < 4) {
        is.seekg(size, std::ios::cur);
        return wtpt;
    }

    wtpt.unityNote = internal::readU16(is);
    wtpt.fineTune = internal::readS16(is);
    if (size > 4) {
        is.seekg(static_cast<std::streamoff>(size - 4), std::ios::cur);
    }
    return wtpt;
}

DlsArticulation parseArtChunk(std::istream& is, uint32_t size) {
    DlsArticulation art;
    if (size < 8) {
        is.seekg(size, std::ios::cur);
        return art;
    }

    art.cbSize = internal::readU32(is);
    art.connectionCount = internal::readU32(is);
    if (art.cbSize < 8 || art.cbSize > size) {
        throw std::runtime_error("DLS articulation header size exceeds chunk size");
    }
    if (art.cbSize > 8 && art.cbSize <= size) {
        is.seekg(static_cast<std::streamoff>(art.cbSize - 8), std::ios::cur);
    }

    const uint32_t availableConnectionBytes = size - art.cbSize;
    if (art.connectionCount > (availableConnectionBytes / 12U)) {
        throw std::runtime_error("DLS articulation connection count exceeds chunk size");
    }

    const std::streampos endPos = is.tellg() + static_cast<std::streamoff>(size - art.cbSize);
    for (uint32_t i = 0; i < art.connectionCount && is.tellg() + std::streamoff(12) <= endPos; ++i) {
        DlsConnection conn;
        conn.source = internal::readU16(is);
        conn.control = internal::readU16(is);
        conn.destination = internal::readU16(is);
        conn.transform = internal::readU16(is);
        conn.scale = internal::readS32(is);
        art.connections.push_back(conn);
    }

    if (is.tellg() < endPos) {
        is.seekg(endPos, std::ios::beg);
    }
    return art;
}

DlsArticulation parseLart(std::istream& is, const internal::RiffChunk& listChunk) {
    DlsArticulation art;
    while (is.tellg() < internal::chunkEnd(listChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == ART1_ID || child.id == internal::ART2_ID) {
            mergeArticulation(art, parseArtChunk(is, child.size));
        }
        internal::seekToChunkEnd(is, child);
    }
    return art;
}

DlsWaveLink parseWlnk(std::istream& is, uint32_t size) {
    DlsWaveLink link;
    if (size >= 12) {
        link.options = internal::readU16(is);
        link.phaseGroup = internal::readU16(is);
        link.channel = internal::readU32(is);
        link.tableIndex = internal::readU32(is);
        if (size > 12) {
            is.seekg(static_cast<std::streamoff>(size - 12), std::ios::cur);
        }
        return link;
    }
    is.seekg(size, std::ios::cur);
    return link;
}

DlsRegionHeader parseRgnh(std::istream& is, uint32_t size) {
    DlsRegionHeader header;
    if (size >= 12) {
        header.keyLow = internal::readU16(is);
        header.keyHigh = internal::readU16(is);
        header.velocityLow = internal::readU16(is);
        header.velocityHigh = internal::readU16(is);
        header.options = internal::readU16(is);
        header.keyGroup = internal::readU16(is);
        if (size > 12) {
            is.seekg(static_cast<std::streamoff>(size - 12), std::ios::cur);
        }
        return header;
    }
    is.seekg(size, std::ios::cur);
    return header;
}

void parseFmt(std::istream& is, uint32_t size, DlsWave& wave) {
    if (size < 16) {
        is.seekg(size, std::ios::cur);
        return;
    }

    wave.formatTag = internal::readU16(is);
    wave.channels = internal::readU16(is);
    wave.sampleRate = internal::readU32(is);
    (void)internal::readU32(is);
    (void)internal::readU16(is);
    wave.bitDepth = internal::readU16(is);
    if (size > 16) {
        is.seekg(static_cast<std::streamoff>(size - 16), std::ios::cur);
    }
}

void parseWaveData(std::istream& is, uint32_t size, DlsWave& wave) {
    const uint32_t bytesPerSample = wave.bitDepth / 8U;
    if (wave.formatTag != 1 || wave.channels == 0 || (wave.bitDepth != 8 && wave.bitDepth != 16) ||
        bytesPerSample == 0) {
        is.seekg(size, std::ios::cur);
        return;
    }

    const uint32_t frameSize = bytesPerSample * wave.channels;
    const uint32_t frameCount = size / frameSize;
    wave.sampleData.reserve(frameCount);
    for (uint32_t frame = 0; frame < frameCount; ++frame) {
        int16_t firstChannel = 0;
        if (wave.bitDepth == 16) {
            firstChannel = internal::readS16(is);
            for (uint16_t channel = 1; channel < wave.channels; ++channel) {
                (void)internal::readS16(is);
            }
        } else {
            firstChannel = static_cast<int16_t>((static_cast<int32_t>(internal::readU8(is)) - 128) << 8);
            for (uint16_t channel = 1; channel < wave.channels; ++channel) {
                (void)internal::readU8(is);
            }
        }
        wave.sampleData.push_back(firstChannel);
    }

    const uint32_t consumed = frameCount * frameSize;
    if (consumed < size) {
        is.seekg(static_cast<std::streamoff>(size - consumed), std::ios::cur);
    }
}

DlsWave parseWave(std::istream& is, const internal::RiffChunk& waveChunk) {
    DlsWave wave;
    while (is.tellg() < internal::chunkEnd(waveChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == FMT_ID) {
            parseFmt(is, child.size, wave);
        } else if (child.id == DATA_ID) {
            parseWaveData(is, child.size, wave);
        } else if (child.id == WSMP_ID) {
            wave.sampleInfo = parseWsmp(is, child.size);
        } else if (child.id == WTPT_ID) {
            const DlsWaveSampleInfo wtpt = parseWtpt(is, child.size);
            if (!wave.sampleInfo) {
                wave.sampleInfo = wtpt;
            } else {
                wave.sampleInfo->unityNote = wtpt.unityNote;
                wave.sampleInfo->fineTune = wtpt.fineTune;
            }
        } else if (child.id == LIST_ID && child.type == INFO_ID) {
            parseInfoList(is, child, wave.info);
        }
        internal::seekToChunkEnd(is, child);
    }
    if (!wave.info.name.empty()) {
        wave.name = wave.info.name;
    }
    return wave;
}

DlsRegion parseRegion(std::istream& is, const internal::RiffChunk& regionChunk) {
    DlsRegion region;
    while (is.tellg() < internal::chunkEnd(regionChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == RGNH_ID) {
            region.header = parseRgnh(is, child.size);
        } else if (child.id == WSMP_ID) {
            region.sampleInfo = parseWsmp(is, child.size);
        } else if (child.id == WLNK_ID) {
            region.waveLink = parseWlnk(is, child.size);
        } else if (child.id == LIST_ID && child.type &&
                   (*child.type == LART_ID || *child.type == LAR2_ID)) {
            mergeArticulation(region.articulation, parseLart(is, child));
        } else if (child.id == ART1_ID || child.id == internal::ART2_ID) {
            mergeArticulation(region.articulation, parseArtChunk(is, child.size));
        }
        internal::seekToChunkEnd(is, child);
    }
    return region;
}

void parseLrgn(std::istream& is, const internal::RiffChunk& listChunk, DlsInstrument& instrument) {
    while (is.tellg() < internal::chunkEnd(listChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == LIST_ID && child.type && (*child.type == RGN_ID || *child.type == RGN2_ID)) {
            instrument.regions.push_back(parseRegion(is, child));
        }
        internal::seekToChunkEnd(is, child);
    }
}

DlsInstrument parseInstrument(std::istream& is, const internal::RiffChunk& insChunk) {
    DlsInstrument instrument;
    std::optional<uint32_t> declaredRegionCount;
    while (is.tellg() < internal::chunkEnd(insChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == INSH_ID && child.size >= 12) {
            declaredRegionCount = internal::readU32(is);
            instrument.bank = internal::readU32(is);
            instrument.program = internal::readU32(is);
        } else if (child.id == LIST_ID && child.type == LRGN_ID) {
            parseLrgn(is, child, instrument);
        } else if (child.id == LIST_ID && child.type &&
                   (*child.type == LART_ID || *child.type == LAR2_ID)) {
            mergeArticulation(instrument.globalArticulation, parseLart(is, child));
        } else if (child.id == LIST_ID && child.type == INFO_ID) {
            parseInfoList(is, child, instrument.info);
        } else if (child.id == ART1_ID || child.id == internal::ART2_ID) {
            mergeArticulation(instrument.globalArticulation, parseArtChunk(is, child.size));
        }
        internal::seekToChunkEnd(is, child);
    }
    if (!instrument.info.name.empty()) {
        instrument.name = instrument.info.name;
    }
    if (declaredRegionCount && *declaredRegionCount != instrument.regions.size()) {
        throw std::runtime_error("DLS insh region count does not match parsed region list");
    }
    return instrument;
}

void parseLins(std::istream& is, const internal::RiffChunk& listChunk, DlsFile& dls) {
    while (is.tellg() < internal::chunkEnd(listChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == LIST_ID && child.type == INS_ID) {
            dls.instruments.push_back(parseInstrument(is, child));
        }
        internal::seekToChunkEnd(is, child);
    }
}

void parseWvpl(std::istream& is, const internal::RiffChunk& listChunk, DlsFile& dls) {
    while (is.tellg() < internal::chunkEnd(listChunk)) {
        const internal::RiffChunk child = internal::readChunk(is);
        if (child.id == LIST_ID && child.type == WAVE_ID) {
            dls.waveOffsets.push_back(
                static_cast<uint32_t>(internal::chunkHeaderStart(child) - listChunk.dataStart));
            dls.waves.push_back(parseWave(is, child));
        }
        internal::seekToChunkEnd(is, child);
    }
}

void parsePtbl(std::istream& is, uint32_t size, DlsFile& dls) {
    if (size < 8) {
        is.seekg(size, std::ios::cur);
        return;
    }

    dls.poolTable.cbSize = internal::readU32(is);
    const uint32_t cueCount = internal::readU32(is);
    if (dls.poolTable.cbSize > 8 && dls.poolTable.cbSize <= size) {
        is.seekg(static_cast<std::streamoff>(dls.poolTable.cbSize - 8), std::ios::cur);
    }

    const uint32_t headerSize = std::max<uint32_t>(8, std::min<uint32_t>(dls.poolTable.cbSize, size));
    const uint32_t availableCueBytes = size - headerSize;
    if (cueCount > (availableCueBytes / sizeof(uint32_t))) {
        throw std::runtime_error("DLS pool table declares more cues than fit in chunk");
    }

    dls.poolTable.cues.clear();
    dls.poolTable.cues.reserve(cueCount);
    for (uint32_t i = 0; i < cueCount; ++i) {
        dls.poolTable.cues.push_back(internal::readU32(is));
    }

    const std::streamoff consumed = std::streamoff(dls.poolTable.cbSize) +
                                    std::streamoff(dls.poolTable.cues.size() * sizeof(uint32_t));
    if (consumed < static_cast<std::streamoff>(size)) {
        is.seekg(static_cast<std::streamoff>(size) - consumed, std::ios::cur);
    }
}

}  // namespace

using namespace internal;

DlsFile DlsParser::parse(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    return parse(file);
}

DlsFile DlsParser::parse(std::istream& is) {
    DlsFile dls;

    const RiffChunk root = readChunk(is);
    if (root.id != RIFF_ID || !root.type || *root.type != DLS_ID) {
        throw std::runtime_error("Not a DLS RIFF file");
    }

    std::optional<uint32_t> declaredInstrumentCount;
    while (is.tellg() < chunkEnd(root)) {
        const RiffChunk chunk = readChunk(is);
        if (chunk.id == LIST_ID && chunk.type == WAVE_LIST_ID) {
            parseWvpl(is, chunk, dls);
        } else if (chunk.id == LIST_ID && chunk.type == LINS_ID) {
            parseLins(is, chunk, dls);
        } else if (chunk.id == LIST_ID && chunk.type == INFO_ID) {
            parseInfoList(is, chunk, dls.info);
        } else if (chunk.id == COLH_ID && chunk.size >= 4) {
            declaredInstrumentCount = readU32(is);
        } else if (chunk.id == PTBL_ID) {
            parsePtbl(is, chunk.size, dls);
        } else if (chunk.id == VERS_ID && chunk.size >= 8) {
            dls.versionMajor = readU32(is);
            dls.versionMinor = readU32(is);
        }
        seekToChunkEnd(is, chunk);
    }

    if (declaredInstrumentCount && *declaredInstrumentCount != dls.instruments.size()) {
        throw std::runtime_error("DLS colh instrument count does not match parsed instrument list");
    }
    if (!dls.poolTable.cues.empty() && dls.poolTable.cues.size() > dls.waves.size()) {
        throw std::runtime_error("DLS pool table contains more cues than parsed wave entries");
    }

    for (size_t i = 0; i < dls.waves.size(); ++i) {
        dls.waves[i].name = fallbackName(dls.waves[i].name, "Wave", i);
    }
    for (size_t i = 0; i < dls.instruments.size(); ++i) {
        dls.instruments[i].name = fallbackName(dls.instruments[i].name, "Instrument", i);
    }
    return dls;
}

}  // namespace dls2sf2
