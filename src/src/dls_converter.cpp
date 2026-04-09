#include "dls2sf2_internal.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <unordered_map>

namespace dls2sf2 {
namespace {

struct ConversionReport {
    std::map<std::string, uint32_t> unsupportedGenerators;
    std::map<std::string, uint32_t> unsupportedModulators;
    std::map<std::string, uint32_t> unrepresentableGenerators;
    std::map<std::string, uint32_t> unrepresentableModulators;
    std::map<std::string, uint32_t> approximatedMappings;

    static bool isUnrepresentableDestination(uint16_t destination) {
        switch (destination) {
            case internal::DLS2_DST_LEFT:
            case internal::DLS2_DST_RIGHT:
            case internal::DLS2_DST_CENTER:
            case internal::DLS2_DST_LEFTREAR:
            case internal::DLS2_DST_RIGHTREAR:
            case internal::DLS2_DST_LFE_CHANNEL:
                return true;
            default:
                return false;
        }
    }

    static bool isUnrepresentableModulationSource(uint16_t source) {
        switch (source) {
            case internal::DLS_SRC_LFO:
            case internal::DLS_SRC_EG1:
            case internal::DLS_SRC_EG2:
            case internal::DLS2_SRC_VIBRATO:
                return true;
            default:
                return false;
        }
    }

    void addUnsupportedGenerator(const DlsConnection& connection) {
        std::ostringstream os;
        os << "gen dst=" << internal::dlsDestinationName(connection.destination);
        if (isUnrepresentableDestination(connection.destination)) {
            unrepresentableGenerators[os.str()]++;
        } else {
            unsupportedGenerators[os.str()]++;
        }
    }

    void addUnsupportedModulator(const DlsConnection& connection) {
        std::ostringstream os;
        os << "mod src=" << internal::dlsSourceName(connection.source)
           << " ctl=" << internal::dlsSourceName(connection.control)
           << " dst=" << internal::dlsDestinationName(connection.destination)
           << " trn=" << internal::dlsTransformName(connection.transform);
        if (isUnrepresentableDestination(connection.destination) ||
            isUnrepresentableModulationSource(connection.source) ||
            isUnrepresentableModulationSource(connection.control)) {
            unrepresentableModulators[os.str()]++;
        } else {
            unsupportedModulators[os.str()]++;
        }
    }

    std::vector<std::string> summarize() const {
        std::vector<std::string> lines;
        for (const auto& [key, count] : approximatedMappings) {
            lines.push_back("Approximated during conversion: " + key + " x" + std::to_string(count));
        }
        for (const auto& [key, count] : unrepresentableGenerators) {
            lines.push_back("Unrepresentable in SF2: " + key + " x" + std::to_string(count));
        }
        for (const auto& [key, count] : unrepresentableModulators) {
            lines.push_back("Unrepresentable in SF2: " + key + " x" + std::to_string(count));
        }
        for (const auto& [key, count] : unsupportedGenerators) {
            lines.push_back("Unsupported DLS generator mapping: " + key + " x" + std::to_string(count));
        }
        for (const auto& [key, count] : unsupportedModulators) {
            lines.push_back("Unsupported DLS modulator mapping: " + key + " x" + std::to_string(count));
        }
        return lines;
    }
};

struct ArticulationConversion {
    std::vector<SF2Generator> generators;
    std::vector<SF2Modulator> modulators;
};

int16_t oneShotReleaseTimecents(const DlsWave& wave) {
    const uint32_t sampleRate = wave.sampleRate == 0 ? 44100 : wave.sampleRate;
    const double sampleSeconds = std::max(0.01, static_cast<double>(wave.sampleData.size()) / sampleRate);
    const double releaseSeconds = std::clamp(sampleSeconds * 8.0, 2.0, 16.0);
    return internal::clampI16(static_cast<int32_t>(std::lround(1200.0 * std::log2(releaseSeconds))));
}

void addLoopApproximationWarnings(const DlsRegion& region, const DlsWave& wave, ConversionReport& report) {
    const DlsWaveSampleInfo* effective = internal::effectiveWsmp(region, wave);
    if (effective == nullptr) {
        return;
    }
    const DlsWaveSampleInfo merged = internal::mergedWsmp(region, wave);
    if (merged.loops.size() > 1) {
        report.approximatedMappings["multiple DLS loops collapsed to first loop"]++;
    }
}

ArticulationConversion convertArticulationWithReport(const DlsArticulation& art, ConversionReport& report) {
    ArticulationConversion out;
    for (const auto& connection : art.connections) {
        const auto gens = internal::convertConstantConnectionGenerators(connection);
        if (!gens.empty()) {
            out.generators.insert(out.generators.end(), gens.begin(), gens.end());
        } else if (connection.source == internal::DLS_SRC_NONE && connection.control == internal::DLS_SRC_NONE) {
            report.addUnsupportedGenerator(connection);
        }

        const auto mod = internal::mapConnectionToModulator(connection);
        if (mod) {
            out.modulators.push_back(*mod);
        } else if (!(connection.source == internal::DLS_SRC_NONE && connection.control == internal::DLS_SRC_NONE) &&
                   internal::convertConstantConnectionGenerators(connection).empty()) {
            report.addUnsupportedModulator(connection);
        }
    }
    return out;
}

std::vector<SF2Generator> effectiveRegionGenerators(const DlsRegion& region, const DlsWave& wave,
                                                    ConversionReport& report) {
    std::vector<SF2Generator> generators;
    const DlsWaveSampleInfo* wsmp = internal::effectiveWsmp(region, wave);
    std::optional<DlsWaveSampleInfo> mergedWsmp;
    generators.push_back({SF2GeneratorType::keyRange,
                          static_cast<int16_t>(internal::makeRangeAmount(region.header.keyLow, region.header.keyHigh))});
    generators.push_back(
        {SF2GeneratorType::velRange,
         static_cast<int16_t>(internal::makeRangeAmount(region.header.velocityLow, region.header.velocityHigh))});

    if (wsmp != nullptr) {
        mergedWsmp = internal::mergedWsmp(region, wave);
        const DlsWaveSampleInfo& merged = *mergedWsmp;
        if (merged.attenuation != 0) {
            generators.push_back(
                {SF2GeneratorType::initialAttenuation, internal::clampI16(merged.attenuation / 65536)});
        }
        if (region.sampleInfo) {
            generators.push_back({SF2GeneratorType::overridingRootKey, static_cast<int16_t>(merged.unityNote)});
            if (merged.fineTune != 0) {
                generators.push_back({SF2GeneratorType::fineTune, merged.fineTune});
            }
        }
        if (!merged.loops.empty()) {
            generators.push_back({SF2GeneratorType::sampleModes, 1});
        }
    }

    if (region.header.keyGroup != 0) {
        generators.push_back({SF2GeneratorType::exclusiveClass, static_cast<int16_t>(region.header.keyGroup)});
    }

    const auto artConversion = convertArticulationWithReport(region.articulation, report);
    generators.insert(generators.end(), artConversion.generators.begin(), artConversion.generators.end());

    const bool hasLoop = mergedWsmp.has_value() && !mergedWsmp->loops.empty();
    const bool selfNonExclusive = (region.header.options & internal::DLS_RGN_OPTION_SELFNONEXCLUSIVE) != 0;
    const bool hasExplicitRelease =
        std::any_of(artConversion.generators.begin(), artConversion.generators.end(),
                    [](const SF2Generator& gen) { return gen.type == SF2GeneratorType::releaseVolEnv; });
    if (selfNonExclusive && !hasLoop && !hasExplicitRelease) {
        generators.push_back({SF2GeneratorType::releaseVolEnv, oneShotReleaseTimecents(wave)});
    }

    return generators;
}

}  // namespace

using namespace internal;

std::vector<SF2Generator> ArticulationConverter::convertGenerators(const DlsArticulation& art) const {
    ConversionReport report;
    return convertArticulationWithReport(art, report).generators;
}

std::vector<SF2Modulator> ArticulationConverter::convertModulators(const DlsArticulation& art) const {
    ConversionReport report;
    return convertArticulationWithReport(art, report).modulators;
}

SF2File Converter::convert(const DlsFile& dls) const {
    if (dls.waves.empty()) {
        throw std::runtime_error("DLS file contains no waves");
    }

    SF2File sf2;
    sf2.INAM = dls.info.name.empty() ? "Converted from DLS" : dls.info.name;
    sf2.ICRD = dls.info.creationDate;
    sf2.IENG = dls.info.engineer;
    sf2.ICMT = dls.info.comment;
    sf2.ICOP = dls.info.copyright;
    sf2.IROM = "DLS";

    std::vector<uint16_t> waveToSampleIndex(dls.waves.size(), 0);
    ConversionReport report;
    for (size_t i = 0; i < dls.waves.size(); ++i) {
        const auto& wave = dls.waves[i];
        if (wave.channels > 1) {
            report.approximatedMappings["multichannel DLS wave collapsed to first channel"]++;
        }
        SF2Sample sample;
        sample.name = fallbackName(wave.name, "Sample", i);
        sample.sampleData = wave.sampleData;
        sample.sampleRate = wave.sampleRate == 0 ? 44100 : wave.sampleRate;
        if (wave.sampleInfo) {
            sample.originalPitch = static_cast<uint8_t>(wave.sampleInfo->unityNote);
            sample.pitchCorrection = clampI8(wave.sampleInfo->fineTune);
            if (!wave.sampleInfo->loops.empty()) {
                const DlsWaveLoop& loop = wave.sampleInfo->loops.front();
                sample.loopStart = loop.loopStart;
                sample.loopEnd = loop.loopStart + loop.loopLength;
            }
        }
        sf2.samples.push_back(sample);
        waveToSampleIndex[i] = static_cast<uint16_t>(i);
    }

    std::unordered_map<uint32_t, uint32_t> waveOffsetToIndex;
    for (size_t i = 0; i < dls.waveOffsets.size(); ++i) {
        waveOffsetToIndex.emplace(dls.waveOffsets[i], static_cast<uint32_t>(i));
    }

    std::vector<std::vector<const DlsWaveLink*>> waveLinks(dls.waves.size());
    for (const auto& instrument : dls.instruments) {
        for (const auto& region : instrument.regions) {
            if (!region.waveLink || region.waveLink->tableIndex >= dls.poolTable.cues.size()) {
                continue;
            }
            const uint32_t poolValue = dls.poolTable.cues[region.waveLink->tableIndex];
            auto waveIt = waveOffsetToIndex.find(poolValue);
            if (waveIt == waveOffsetToIndex.end()) {
                continue;
            }
            waveLinks[waveIt->second].push_back(&*region.waveLink);
            const DlsWaveSampleInfo* wsmp = effectiveWsmp(region, dls.waves[waveIt->second]);
            if (wsmp != nullptr) {
                const DlsWaveSampleInfo merged = mergedWsmp(region, dls.waves[waveIt->second]);
                sf2.samples[waveIt->second].originalPitch = static_cast<uint8_t>(merged.unityNote);
                sf2.samples[waveIt->second].pitchCorrection = clampI8(merged.fineTune);
                if (!merged.loops.empty()) {
                    sf2.samples[waveIt->second].loopStart = merged.loops.front().loopStart;
                    sf2.samples[waveIt->second].loopEnd =
                        merged.loops.front().loopStart + merged.loops.front().loopLength;
                }
            }
        }
    }

    for (size_t i = 0; i < waveLinks.size(); ++i) {
        if (waveLinks[i].empty()) {
            continue;
        }

        // Default to mono unless a valid left/right pair is found.
        sf2.samples[i].sampleType = 1;
        sf2.samples[i].sampleLink = 0;
    }

    for (size_t i = 0; i < waveLinks.size(); ++i) {
        if (waveLinks[i].empty() || sf2SampleTypeFromLink(waveLinks[i].front()) != 4) {
            continue;
        }

        const DlsWaveLink* left = waveLinks[i].front();
        for (size_t j = 0; j < waveLinks.size(); ++j) {
            if (i == j || waveLinks[j].empty() || sf2SampleTypeFromLink(waveLinks[j].front()) != 2) {
                continue;
            }

            const DlsWaveLink* right = waveLinks[j].front();
            if (left->phaseGroup != right->phaseGroup) {
                continue;
            }

            sf2.samples[i].sampleType = 4;
            sf2.samples[j].sampleType = 2;
            sf2.samples[i].sampleLink = static_cast<uint16_t>(j);
            sf2.samples[j].sampleLink = static_cast<uint16_t>(i);
            break;
        }
    }

    for (size_t instIndex = 0; instIndex < dls.instruments.size(); ++instIndex) {
        const auto& instrument = dls.instruments[instIndex];

        SF2Instrument sf2Inst;
        sf2Inst.name = fallbackName(instrument.name, "Instrument", instIndex);
        sf2Inst.bagIndex = static_cast<uint16_t>(sf2.instrumentBags.size());
        sf2.instruments.push_back(sf2Inst);

        if (!instrument.globalArticulation.connections.empty()) {
            SF2InstrumentBag globalBag;
            globalBag.genIndex = static_cast<uint16_t>(sf2.instrumentGenerators.size());
            globalBag.modIndex = static_cast<uint16_t>(sf2.instrumentModulators.size());
            sf2.instrumentBags.push_back(globalBag);

            const auto artConversion = convertArticulationWithReport(instrument.globalArticulation, report);
            sf2.instrumentGenerators.insert(sf2.instrumentGenerators.end(), artConversion.generators.begin(),
                                            artConversion.generators.end());
            sf2.instrumentModulators.insert(sf2.instrumentModulators.end(), artConversion.modulators.begin(),
                                            artConversion.modulators.end());
        }

        for (const auto& region : instrument.regions) {
            if (!region.waveLink || region.waveLink->tableIndex >= dls.poolTable.cues.size()) {
                throw std::runtime_error("Region references invalid pool table index");
            }

            const uint32_t poolValue = dls.poolTable.cues[region.waveLink->tableIndex];
            auto waveIt = waveOffsetToIndex.find(poolValue);
            if (waveIt == waveOffsetToIndex.end()) {
                if (poolValue < dls.waves.size()) {
                    waveIt = waveOffsetToIndex.find(dls.waveOffsets[poolValue]);
                }
            }
            if (waveIt == waveOffsetToIndex.end()) {
                throw std::runtime_error("Pool table entry references unknown wave offset");
            }
            const uint32_t waveIndex = waveIt->second;
            addLoopApproximationWarnings(region, dls.waves[waveIndex], report);

            SF2InstrumentBag bag;
            bag.genIndex = static_cast<uint16_t>(sf2.instrumentGenerators.size());
            bag.modIndex = static_cast<uint16_t>(sf2.instrumentModulators.size());
            sf2.instrumentBags.push_back(bag);

            auto generators = effectiveRegionGenerators(region, dls.waves[waveIndex], report);
            generators.push_back({SF2GeneratorType::sampleID, static_cast<int16_t>(waveToSampleIndex[waveIndex])});
            sf2.instrumentGenerators.insert(sf2.instrumentGenerators.end(), generators.begin(), generators.end());

            const auto artConversion = convertArticulationWithReport(region.articulation, report);
            sf2.instrumentModulators.insert(sf2.instrumentModulators.end(), artConversion.modulators.begin(),
                                            artConversion.modulators.end());
        }

        SF2Preset preset;
        preset.name = sf2Inst.name;
        preset.preset = static_cast<uint16_t>(instrument.program & 0x7F);
        preset.bank = (instrument.bank & 0x80000000U) != 0U
                          ? 128
                          : static_cast<uint16_t>(instrument.bank & 0x7FFFU);
        preset.bagIndex = static_cast<uint16_t>(sf2.presetBags.size());
        sf2.presets.push_back(preset);

        SF2PresetBag pbag;
        pbag.genIndex = static_cast<uint16_t>(sf2.presetGenerators.size());
        pbag.modIndex = static_cast<uint16_t>(sf2.presetModulators.size());
        sf2.presetBags.push_back(pbag);
        sf2.presetGenerators.push_back(
            {SF2GeneratorType::instrument, static_cast<int16_t>(sf2.instruments.size() - 1)});
    }

    if (sf2.presets.empty()) {
        SF2Instrument sf2Inst;
        sf2Inst.name = "Instrument 0";
        sf2Inst.bagIndex = static_cast<uint16_t>(sf2.instrumentBags.size());
        sf2.instruments.push_back(sf2Inst);

        SF2InstrumentBag bag;
        bag.genIndex = static_cast<uint16_t>(sf2.instrumentGenerators.size());
        bag.modIndex = static_cast<uint16_t>(sf2.instrumentModulators.size());
        sf2.instrumentBags.push_back(bag);
        sf2.instrumentGenerators.push_back({SF2GeneratorType::sampleID, 0});

        SF2Preset preset;
        preset.name = "Preset 0";
        preset.bagIndex = static_cast<uint16_t>(sf2.presetBags.size());
        sf2.presets.push_back(preset);

        SF2PresetBag pbag;
        pbag.genIndex = static_cast<uint16_t>(sf2.presetGenerators.size());
        pbag.modIndex = static_cast<uint16_t>(sf2.presetModulators.size());
        sf2.presetBags.push_back(pbag);
        sf2.presetGenerators.push_back({SF2GeneratorType::instrument, 0});
    }

    sf2.warnings = report.summarize();
    if (!sf2.warnings.empty()) {
        std::ostringstream os;
        if (!sf2.ICMT.empty()) {
            os << sf2.ICMT << "\n";
        }
        for (size_t i = 0; i < sf2.warnings.size(); ++i) {
            if (i != 0) {
                os << "\n";
            }
            os << sf2.warnings[i];
        }
        sf2.ICMT = os.str();
    }

    return sf2;
}

}  // namespace dls2sf2
