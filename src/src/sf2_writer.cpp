#include "dls2sf2_internal.hpp"

namespace dls2sf2 {

using namespace internal;

void SF2Writer::write(const std::string& filename, const SF2File& sf2) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Cannot create file: " + filename);
    }
    write(file, sf2);
}

void SF2Writer::write(std::ostream& os, const SF2File& input) {
    SF2File sf2 = input;
    std::vector<int16_t> smplData = buildSmplData(sf2.samples);

    writeU32(os, RIFF_ID);
    const std::streampos riffSizePos = os.tellp();
    writeU32(os, 0);
    writeU32(os, SFBK_ID);

    ChunkWriter infoList(os);
    infoList.beginList(INFO_ID);
    writeSimpleChunk(os, IFIL_ID, [&]() {
        writeU16(os, sf2.ifilMajor);
        writeU16(os, sf2.ifilMinor);
    });
    auto writeInfoString = [&](uint32_t id, const std::string& value) {
        if (value.empty()) {
            return;
        }
        writeSimpleChunk(os, id, [&]() {
            os.write(value.c_str(), static_cast<std::streamsize>(value.size()));
            os.put('\0');
        });
    };
    writeInfoString(ISNG_ID, sf2.isng);
    writeInfoString(INAM_ID, sf2.INAM);
    writeInfoString(IROM_ID, sf2.IROM);
    writeInfoString(ICRD_ID, sf2.ICRD);
    writeInfoString(IENG_ID, sf2.IENG);
    writeInfoString(ICMT_ID, sf2.ICMT);
    writeInfoString(ICOP_ID, sf2.ICOP);
    writeInfoString(ISFT_ID, sf2.ISFT);
    infoList.end();

    ChunkWriter sdtaList(os);
    sdtaList.beginList(SDTA_ID);
    writeSimpleChunk(os, SMPL_ID, [&]() { writeVector(os, smplData); });
    sdtaList.end();

    ChunkWriter pdtaList(os);
    pdtaList.beginList(PDTA_ID);
    writeSimpleChunk(os, PHDR_ID, [&]() {
        for (const auto& preset : sf2.presets) {
            PhdrRecord record{};
            writeFixedName(record.name, preset.name);
            record.preset = preset.preset;
            record.bank = preset.bank;
            record.bagIndex = preset.bagIndex;
            record.library = preset.library;
            record.genre = preset.genre;
            record.morphology = preset.morphology;
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        PhdrRecord terminal{};
        writeFixedName(terminal.name, "EOP");
        terminal.preset = 0xFFFF;
        terminal.bank = 0xFFFF;
        terminal.bagIndex = static_cast<uint16_t>(sf2.presetBags.size());
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, PBAG_ID, [&]() {
        for (const auto& bag : sf2.presetBags) {
            BagRecord record{bag.genIndex, bag.modIndex};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        BagRecord terminal{static_cast<uint16_t>(sf2.presetGenerators.size()),
                           static_cast<uint16_t>(sf2.presetModulators.size())};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, PMOD_ID, [&]() {
        for (const auto& mod : sf2.presetModulators) {
            ModRecord record{mod.srcOper, mod.destOper, mod.amount, mod.amtSrcOper, mod.transOper};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        ModRecord terminal{};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, PGEN_ID, [&]() {
        for (const auto& gen : sf2.presetGenerators) {
            GenRecord record{static_cast<uint16_t>(gen.type), gen.amount};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        GenRecord terminal{};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, INST_ID, [&]() {
        for (const auto& instrument : sf2.instruments) {
            InstRecord record{};
            writeFixedName(record.name, instrument.name);
            record.bagIndex = instrument.bagIndex;
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        InstRecord terminal{};
        writeFixedName(terminal.name, "EOI");
        terminal.bagIndex = static_cast<uint16_t>(sf2.instrumentBags.size());
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, IBAG_ID, [&]() {
        for (const auto& bag : sf2.instrumentBags) {
            BagRecord record{bag.genIndex, bag.modIndex};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        BagRecord terminal{static_cast<uint16_t>(sf2.instrumentGenerators.size()),
                           static_cast<uint16_t>(sf2.instrumentModulators.size())};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, IMOD_ID, [&]() {
        for (const auto& mod : sf2.instrumentModulators) {
            ModRecord record{mod.srcOper, mod.destOper, mod.amount, mod.amtSrcOper, mod.transOper};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        ModRecord terminal{};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, IGEN_ID, [&]() {
        for (const auto& gen : sf2.instrumentGenerators) {
            GenRecord record{static_cast<uint16_t>(gen.type), gen.amount};
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        GenRecord terminal{};
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    writeSimpleChunk(os, SHDR_ID, [&]() {
        for (const auto& sample : sf2.samples) {
            SampleHeaderRecord record{};
            writeFixedName(record.name, sample.name);
            record.start = sample.start;
            record.end = sample.end;
            record.startLoop = sample.loopStart;
            record.endLoop = sample.loopEnd;
            record.sampleRate = sample.sampleRate;
            record.originalPitch = sample.originalPitch;
            record.pitchCorrection = sample.pitchCorrection;
            record.sampleLink = sample.sampleLink;
            record.sampleType = sample.sampleType;
            os.write(reinterpret_cast<const char*>(&record), sizeof(record));
        }
        SampleHeaderRecord terminal{};
        writeFixedName(terminal.name, "EOS");
        terminal.start = static_cast<uint32_t>(smplData.size());
        terminal.end = static_cast<uint32_t>(smplData.size());
        terminal.startLoop = static_cast<uint32_t>(smplData.size());
        terminal.endLoop = static_cast<uint32_t>(smplData.size());
        terminal.sampleRate = 44100;
        os.write(reinterpret_cast<const char*>(&terminal), sizeof(terminal));
    });
    pdtaList.end();

    const std::streampos endPos = os.tellp();
    const uint32_t riffSize = static_cast<uint32_t>(endPos - riffSizePos - std::streamoff(4));
    os.seekp(riffSizePos);
    writeU32(os, riffSize);
    os.seekp(endPos);
}

}  // namespace dls2sf2
