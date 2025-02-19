#ifndef OPENMW_ESM_AIPACKAGE_H
#define OPENMW_ESM_AIPACKAGE_H

#include <string>
#include <vector>

#include "components/esm/esmcommon.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

#pragma pack(push)
#pragma pack(1)

    struct AIData
    {
        uint16_t mHello; // This is the base value for greeting distance [0, 65535]
        unsigned char mFight, mFlee, mAlarm; // These are probabilities [0, 100]
        char mU1, mU2, mU3; // Unknown values
        int32_t mServices; // See the Services enum

        void blank();
        ///< Set record to default state (does not touch the ID).
    }; // 12 bytes

    struct AIWander
    {
        int16_t mDistance;
        int16_t mDuration;
        unsigned char mTimeOfDay;
        unsigned char mIdle[8];
        unsigned char mShouldRepeat;
    };

    struct AITravel
    {
        float mX, mY, mZ;
        unsigned char mShouldRepeat;
        unsigned char mPadding[3];
    };

    struct AITarget
    {
        float mX, mY, mZ;
        int16_t mDuration;
        NAME32 mId;
        unsigned char mShouldRepeat;
        unsigned char mPadding;
    };

    struct AIActivate
    {
        NAME32 mName;
        unsigned char mShouldRepeat;
    };

#pragma pack(pop)

    enum AiPackageType : std::uint32_t
    {
        AI_Wander = 0x575f4941,
        AI_Travel = 0x545f4941,
        AI_Follow = 0x465f4941,
        AI_Escort = 0x455f4941,
        AI_Activate = 0x415f4941,
    };

    inline constexpr std::uint32_t AI_CNDT = 0x54444e43;

    /// \note Used for storaging packages in a single container
    /// w/o manual memory allocation accordingly to policy standards
    struct AIPackage
    {
        AiPackageType mType;

        // Anonymous union
        union
        {
            AIWander mWander;
            AITravel mTravel;
            AITarget mTarget;
            AIActivate mActivate;
        };

        /// \note for AITarget only, placed here to stick with union,
        /// overhead should be not so awful
        std::string mCellName;
    };

    struct AIPackageList
    {
        std::vector<AIPackage> mList;

        /// Add a single AIPackage, assumes subrecord name was already read
        void add(ESMReader& esm);

        void save(ESMWriter& esm) const;
    };
}

#endif
