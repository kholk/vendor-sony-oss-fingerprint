#pragma once

#include <IonBuffer.h>
#include <arpa/inet.h>
#include <hardware/hw_auth_token.h>
#include <string.h>
#include <algorithm>
#include <vector>
#include "QSEEKeymasterTrustlet.h"
#include "QSEETrustlet.h"

namespace egistec::ganges {

enum class CommandId : uint32_t {
    SetMasterKey = 0,
    InitializeAlgo = 1,
    InitializeSensor = 2,
    Calibrate = 6,

    GetImage = 0x8,

    InitializeEnroll = 0xb,
    Enroll = 0xc,
    FinalizeEnroll = 0xd,
    SaveEnrolledPrint = 0xe,

    GetPrintIds = 0x16,
    SetWorkMode = 0x17,
    SetUserDataPath = 0x18,
    SetDataPath = 0x19,
    CheckSecureId = 0x1e,
    CheckAuthToken = 0x1f,
    GetAuthenticatorId = 0x20,

    IsFingerLost = 0x25,

    OpenSpi = 0x29,
    CloseSpi = 0x2a,
};

enum class ImageResult : uint32_t {
    Good = 0,
    Detected1 = 1,
    TooFast = 2,
    Detected3 = 3,
    Lost = 6,
    ImagerDirty = 7,
    Partial = 8,
    Nothing = 10,
    DirtOnSensor = 0xd,
};

/**
 * The datastructure through which this userspace HAL communicates with the TZ app.
 */
typedef struct {
    uint32_t process;
    CommandId command;
    uint32_t gid;
    uint32_t fid;
    uint32_t buffer_size;
    char data[];
} trustlet_buffer_t;

static_assert(sizeof(trustlet_buffer_t) == 0x14, "trustlet_buffer_t not of expected size!");

typedef struct {
    uint32_t process_id;
    uint32_t no_extra_buffer;
    uint32_t unk0;
    uint32_t extra_buffer_size;
    uint32_t ret_val;
    union {
        struct {
            uint32_t unk1;
            uint32_t unk2;
            uint32_t extra_flags;
        };
        char data[];
    };
} base_transaction_t;

static_assert(sizeof(base_transaction_t) == 0x20, "");
static_assert(offsetof(base_transaction_t, extra_buffer_size) == 0xc, "");
static_assert(offsetof(base_transaction_t, ret_val) == 0x10, "");
static_assert(offsetof(base_transaction_t, data) == 0x14, "");
static_assert(offsetof(base_transaction_t, extra_flags) == 0x1c, "");

typedef struct {
    ImageResult status;
    int percentage;
    int dx;
    int dy;
    int unk0;
    int score;
    int unk1;
    int unk2;
} enroll_result_t;

static_assert(sizeof(enroll_result_t) == 0x20, "");

class EGISAPTrustlet : public QSEETrustlet {
   protected:
    class API {
        // TODO: Could be a templated class defined in QSEETrustlet.

        static inline constexpr auto RequestOffset = 0x5c;
        static inline constexpr auto ResponseOffset = 0x14;

        QSEETrustlet::LockedIONBuffer mLockedBuffer;

       public:
        inline API(QSEETrustlet::LockedIONBuffer &&lockedBuffer) : mLockedBuffer(std::move(lockedBuffer)) {
        }

        inline trustlet_buffer_t &GetRequest() {
            return *reinterpret_cast<trustlet_buffer_t *>((ptrdiff_t)*mLockedBuffer + RequestOffset);
        }

        inline trustlet_buffer_t &GetResponse() {
            return *reinterpret_cast<trustlet_buffer_t *>((ptrdiff_t)*mLockedBuffer + ResponseOffset);
        }

        inline base_transaction_t &Base() {
            return *reinterpret_cast<base_transaction_t *>(*mLockedBuffer);
        }

        inline base_transaction_t &PrepareBase(uint32_t process) {
            auto &base = Base();
            memset(&base, 0, sizeof(base));
            base.process_id = process;
            return base;
        }

        inline void MoveResponseToRequest() {
            memmove(&GetRequest(), &GetResponse(), sizeof(trustlet_buffer_t));
        }

        static inline constexpr size_t BufferSize() {
            return sizeof(trustlet_buffer_t) + std::max(RequestOffset, ResponseOffset);
        }

        friend class EGISAPTrustlet;
    };

   public:
    EGISAPTrustlet();

    int SendCommand(API &);
    int SendCommand(API &, CommandId, uint32_t gid = 0);
    int SendCommand(CommandId, uint32_t gid = 0);
    int SendModifiedCommand(API &, IonBuffer &);
    int SendModifiedCommand(API &, IonBuffer &, CommandId, uint32_t gid = 0);
    int SendModifiedCommand(IonBuffer &, CommandId, uint32_t gid = 0);
    int SendDataCommand(API &, CommandId, const void *data, size_t length, uint32_t gid = 0);
    int SendDataCommand(CommandId, const void *data, size_t length, uint32_t gid = 0);
    API GetLockedAPI();

    int Calibrate();
    int GetPrintIds(uint32_t gid, std::vector<uint32_t> &);
    int InitializeAlgo();
    int InitializeSensor();
    int SetDataPath(const char *);
    int SetMasterKey(const MasterKey &);
    int SetUserDataPath(uint32_t gid, const char *);
    int SetWorkMode(uint32_t);
    uint64_t GetAuthenticatorId();

    int GetImage(ImageResult &quality);
    int IsFingerLost(uint32_t timeout, ImageResult &status);
    int SetSpiState(uint32_t on);

    // Enrolling
    int CheckAuthToken(const hw_auth_token_t &);
    int CheckSecureId(uint32_t gid, uint64_t user_id);
    int Enroll(uint32_t gid, uint32_t fid, enroll_result_t &);
    int GetNewPrintId(uint32_t gid, uint32_t &new_print_id);
    int InitializeEnroll();
    int SaveEnrolledPrint(uint32_t gid, uint64_t fid);
    int FinalizeEnroll();
};

}  // namespace egistec::ganges
