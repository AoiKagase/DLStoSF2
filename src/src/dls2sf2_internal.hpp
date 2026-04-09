#pragma once

#include "dls2sf2.hpp"

#include <array>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

namespace dls2sf2::internal {

constexpr uint32_t ART2_ID = makeFourCC('a', 'r', 't', '2');
constexpr uint16_t DLS_SRC_NONE = 0x0000;
constexpr uint16_t DLS_SRC_LFO = 0x0001;
constexpr uint16_t DLS_SRC_KEYONVELOCITY = 0x0002;
constexpr uint16_t DLS_SRC_KEYNUMBER = 0x0003;
constexpr uint16_t DLS_SRC_EG1 = 0x0004;
constexpr uint16_t DLS_SRC_EG2 = 0x0005;
constexpr uint16_t DLS_SRC_PITCHWHEEL = 0x0006;
constexpr uint16_t DLS2_SRC_POLYPRESSURE = 0x0007;
constexpr uint16_t DLS2_SRC_CHANNELPRESSURE = 0x0008;
constexpr uint16_t DLS2_SRC_VIBRATO = 0x0009;
constexpr uint16_t DLS2_SRC_MONOPRESSURE = 0x000A;
constexpr uint16_t DLS_SRC_CC1 = 0x0081;
constexpr uint16_t DLS_SRC_CC7 = 0x0087;
constexpr uint16_t DLS_SRC_CC10 = 0x008A;
constexpr uint16_t DLS_SRC_CC11 = 0x008B;
constexpr uint16_t DLS2_SRC_CC91 = 0x00DB;
constexpr uint16_t DLS2_SRC_CC93 = 0x00DD;
constexpr uint16_t DLS_SRC_RPN0 = 0x0100;
constexpr uint16_t DLS_SRC_RPN1 = 0x0101;
constexpr uint16_t DLS_SRC_RPN2 = 0x0102;
constexpr uint16_t DLS_TRN_NONE = 0x0000;
constexpr uint16_t DLS_TRN_CONCAVE = 0x0001;
constexpr uint16_t DLS2_TRN_CONVEX = 0x0002;
constexpr uint16_t DLS2_TRN_SWITCH = 0x0003;
constexpr uint16_t DLS_RGN_OPTION_SELFNONEXCLUSIVE = 0x0001;
constexpr uint16_t DLS_WAVELINK_OPTION_MULTICHANNEL = 0x0002;
constexpr uint16_t DLS_WAVELINK_OPTION_PHASE_MASTER = 0x0001;
constexpr uint32_t DLS_CHANNEL_LEFT = 0x00000001;
constexpr uint32_t DLS_CHANNEL_RIGHT = 0x00000002;
constexpr uint16_t DLS_DST_NONE = 0x0000;
constexpr uint16_t DLS_DST_ATTENUATION = 0x0001;
constexpr uint16_t DLS_DST_PITCH = 0x0003;
constexpr uint16_t DLS_DST_PAN = 0x0004;
constexpr uint16_t DLS2_DST_KEYNUMBER = 0x0005;
constexpr uint16_t DLS2_DST_GAIN = 0x0001;
constexpr uint16_t DLS2_DST_LEFT = 0x0010;
constexpr uint16_t DLS2_DST_RIGHT = 0x0011;
constexpr uint16_t DLS2_DST_CENTER = 0x0012;
constexpr uint16_t DLS2_DST_LEFTREAR = 0x0013;
constexpr uint16_t DLS2_DST_RIGHTREAR = 0x0014;
constexpr uint16_t DLS2_DST_LFE_CHANNEL = 0x0015;
constexpr uint16_t DLS2_DST_CHORUS = 0x0080;
constexpr uint16_t DLS2_DST_REVERB = 0x0081;
constexpr uint16_t DLS_DST_LFO_FREQUENCY = 0x0104;
constexpr uint16_t DLS_DST_LFO_STARTDELAY = 0x0105;
constexpr uint16_t DLS_DST_VIB_FREQUENCY = 0x0114;
constexpr uint16_t DLS_DST_VIB_STARTDELAY = 0x0115;
constexpr uint16_t DLS_DST_EG1_ATTACKTIME = 0x0206;
constexpr uint16_t DLS_DST_EG1_DECAYTIME = 0x0207;
constexpr uint16_t DLS_DST_EG1_RELEASETIME = 0x0209;
constexpr uint16_t DLS_DST_EG1_SUSTAINLEVEL = 0x020A;
constexpr uint16_t DLS_DST_EG1_DELAYTIME = 0x020B;
constexpr uint16_t DLS_DST_EG1_HOLDTIME = 0x020C;
constexpr uint16_t DLS_DST_EG2_ATTACKTIME = 0x030A;
constexpr uint16_t DLS_DST_EG2_DECAYTIME = 0x030B;
constexpr uint16_t DLS_DST_EG2_RELEASETIME = 0x030D;
constexpr uint16_t DLS_DST_EG2_SUSTAINLEVEL = 0x030E;
constexpr uint16_t DLS_DST_EG2_DELAYTIME = 0x030F;
constexpr uint16_t DLS_DST_EG2_HOLDTIME = 0x0310;
constexpr uint16_t DLS_DST_EG1_KEYNUMTOHOLD = 0x0208;
constexpr uint16_t DLS_DST_EG2_KEYNUMTODECAY = 0x030C;
constexpr uint16_t DLS_DST_EG2_KEYNUMTOHOLD = 0x0311;
constexpr uint16_t DLS_DST_EG2_KEYNUMTODECAY2 = 0x0312;
constexpr uint16_t DLS_DST_FILTER_CUTOFF = 0x0500;
constexpr uint16_t DLS_DST_FILTER_Q = 0x0501;

struct RiffChunk {
    uint32_t id = 0;
    uint32_t size = 0;
    std::optional<uint32_t> type;
    std::streampos dataStart = {};
};

#pragma pack(push, 1)

struct SampleHeaderRecord {
    char name[20];
    uint32_t start;
    uint32_t end;
    uint32_t startLoop;
    uint32_t endLoop;
    uint32_t sampleRate;
    uint8_t originalPitch;
    int8_t pitchCorrection;
    uint16_t sampleLink;
    uint16_t sampleType;
};

struct PhdrRecord {
    char name[20];
    uint16_t preset;
    uint16_t bank;
    uint16_t bagIndex;
    uint32_t library;
    uint32_t genre;
    uint32_t morphology;
};

struct InstRecord {
    char name[20];
    uint16_t bagIndex;
};

struct BagRecord {
    uint16_t genIndex;
    uint16_t modIndex;
};

struct GenRecord {
    uint16_t oper;
    int16_t amount;
};

struct ModRecord {
    uint16_t srcOper;
    uint16_t destOper;
    int16_t amount;
    uint16_t amtSrcOper;
    uint16_t transOper;
};

#pragma pack(pop)

static_assert(sizeof(SampleHeaderRecord) == 46, "SF2 sample header must be 46 bytes");
static_assert(sizeof(PhdrRecord) == 38, "SF2 phdr record must be 38 bytes");
static_assert(sizeof(InstRecord) == 22, "SF2 inst record must be 22 bytes");
static_assert(sizeof(BagRecord) == 4, "SF2 bag record must be 4 bytes");
static_assert(sizeof(GenRecord) == 4, "SF2 gen record must be 4 bytes");
static_assert(sizeof(ModRecord) == 10, "SF2 mod record must be 10 bytes");

uint32_t readU32(std::istream& is);
uint16_t readU16(std::istream& is);
int16_t readS16(std::istream& is);
int32_t readS32(std::istream& is);
uint8_t readU8(std::istream& is);
std::string readBytes(std::istream& is, size_t size);
std::string trimNullTerminated(const std::string& value);
void skipPadding(std::istream& is, uint32_t size);
void seekToChunkEnd(std::istream& is, const RiffChunk& chunk);
RiffChunk readChunk(std::istream& is);
std::streampos chunkEnd(const RiffChunk& chunk);
std::streampos chunkHeaderStart(const RiffChunk& chunk);

void writeU32(std::ostream& os, uint32_t value);
void writeU16(std::ostream& os, uint16_t value);
void writeFixedName(char* dst, const std::string& src);

template <typename T>
void writeVector(std::ostream& os, const std::vector<T>& values) {
    if (!values.empty()) {
        os.write(reinterpret_cast<const char*>(values.data()),
                 static_cast<std::streamsize>(sizeof(T) * values.size()));
    }
}

class ChunkWriter {
public:
    explicit ChunkWriter(std::ostream& os);
    void beginChunk(uint32_t id);
    void beginList(uint32_t type);
    void end();

private:
    std::ostream& os_;
    std::streampos sizePos_;
};

int16_t clampI16(int32_t value);
int8_t clampI8(int32_t value);
uint16_t makeRangeAmount(uint16_t low, uint16_t high);
uint16_t sf2SampleTypeFromLink(const DlsWaveLink* link);
const DlsWaveSampleInfo* effectiveWsmp(const DlsRegion& region, const DlsWave& wave);
DlsWaveSampleInfo mergedWsmp(const DlsRegion& region, const DlsWave& wave);
std::string fallbackName(const std::string& preferred, const std::string& prefix, size_t index);
int16_t dlsFixedToSf2Timecents(int32_t value);
int16_t dlsFixedToSf2Cents(int32_t value);
int16_t dlsFixedPercentToSf2ModSustain(int32_t value);
int16_t dlsFixedPercentToSf2VolSustain(int32_t value);
int16_t scaleGeneratorAmount(const DlsConnection& connection, SF2GeneratorType type);
std::vector<SF2Generator> convertSourceConnectionGenerators(const DlsConnection& connection);
std::optional<SF2Generator> mapSimpleConnection(const DlsConnection& connection);
std::optional<uint16_t> mapDestinationToGenerator(uint16_t destination);
std::optional<uint16_t> mapDlsControlSource(uint16_t source, uint16_t transform, bool negativeFromScale);
std::optional<SF2Modulator> mapConnectionToModulator(const DlsConnection& connection);
std::vector<SF2Generator> convertConstantConnectionGenerators(const DlsConnection& connection);
std::vector<int16_t> buildSmplData(std::vector<SF2Sample>& samples);
std::string dlsSourceName(uint16_t value);
std::string dlsDestinationName(uint16_t value);
std::string dlsTransformName(uint16_t value);

template <typename RecordWriter>
void writeSimpleChunk(std::ostream& os, uint32_t id, RecordWriter writer) {
    ChunkWriter chunk(os);
    chunk.beginChunk(id);
    writer();
    chunk.end();
}

}  // namespace dls2sf2::internal
