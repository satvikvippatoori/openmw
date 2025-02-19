/*
  Copyright (C) 2016-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#include "loadnpc.hpp"

#include <cstring>
#include <stdexcept>
#include <string> // getline

#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Npc::load(ESM4::Reader& reader)
{
    mId = reader.getFormIdFromHeader();
    mFlags = reader.hdr().record.flags;

    std::uint32_t esmVer = reader.esmVersion();
    mIsTES4 = (esmVer == ESM::VER_080 || esmVer == ESM::VER_100) && !reader.hasFormVersion();
    mIsFONV = esmVer == ESM::VER_132 || esmVer == ESM::VER_133 || esmVer == ESM::VER_134;
    // mIsTES5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_170; // WARN: FO3 is also VER_094

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
                reader.getZString(mEditorId);
                break;
            case ESM4::SUB_MODL:
                reader.getZString(mModel);
                break; // not for TES5, see Race
            case ESM4::SUB_FULL:
                reader.getLocalizedString(mFullName);
                break;
            case ESM4::SUB_CNTO:
            {
                static InventoryItem inv; // FIXME: use unique_ptr here?
                reader.get(inv);
                reader.adjustFormId(inv.item);
                mInventory.push_back(inv);
                break;
            }
            case ESM4::SUB_SPLO:
                reader.getFormId(mSpell.emplace_back());
                break;
            case ESM4::SUB_PKID:
                reader.getFormId(mAIPackages.emplace_back());
                break;
            case ESM4::SUB_SNAM:
            {
                // FO4, FO76
                if (subHdr.dataSize == 5)
                    reader.get(&mFaction, 5);
                else
                    reader.get(mFaction);
                reader.adjustFormId(mFaction.faction);
                break;
            }
            case ESM4::SUB_RNAM:
                reader.getFormId(mRace);
                break;
            case ESM4::SUB_CNAM:
                reader.getFormId(mClass);
                break;
            case ESM4::SUB_HNAM:
                reader.getFormId(mHair);
                break; // not for TES5
            case ESM4::SUB_ENAM:
                reader.getFormId(mEyes);
                break;
            //
            case ESM4::SUB_INAM:
                reader.getFormId(mDeathItem);
                break;
            case ESM4::SUB_SCRI:
                reader.getFormId(mScriptId);
                break;
            //
            case ESM4::SUB_AIDT:
            {
                if (subHdr.dataSize != 12)
                {
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                    break;
                }

                reader.get(mAIData); // TES4
                break;
            }
            case ESM4::SUB_ACBS:
            {
                switch (subHdr.dataSize)
                {
                    case 20: // FO4
                        mIsFO4 = true;
                        [[fallthrough]];
                    case 16: // TES4
                    case 24: // FO3/FNV, TES5
                        reader.get(&mBaseConfig, subHdr.dataSize);
                        break;
                    default:
                        reader.skipSubRecordData();
                        break;
                }
                break;
            }
            case ESM4::SUB_DATA:
            {
                if (subHdr.dataSize == 0)
                    break;

                if (subHdr.dataSize == 33)
                    reader.get(&mData, 33); // FIXME: check packing
                else // FIXME FO3
                    reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_ZNAM:
                reader.getFormId(mCombatStyle);
                break;
            case ESM4::SUB_CSCR:
                reader.getFormId(mSoundBase);
                break;
            case ESM4::SUB_CSDI:
                reader.getFormId(mSound);
                break;
            case ESM4::SUB_CSDC:
                reader.get(mSoundChance);
                break;
            case ESM4::SUB_WNAM:
            {
                // FIXME: should be read into mWornArmor for FO4
                if (reader.esmVersion() == ESM::VER_094 || reader.esmVersion() == ESM::VER_170)
                    reader.getFormId(mWornArmor);
                else
                    reader.get(mFootWeight);
                break;
            }
            case ESM4::SUB_MODB:
                reader.get(mBoundRadius);
                break;
            case ESM4::SUB_KFFZ:
            {
                // Seems to be only below 3, and only happens 3 times while loading TES4:
                //   Forward_SheogorathWithCane.kf
                //   TurnLeft_SheogorathWithCane.kf
                //   TurnRight_SheogorathWithCane.kf
                if (!reader.getZeroTerminatedStringArray(mKf))
                    throw std::runtime_error("NPC_ KFFZ data read error");
                break;
            }
            case ESM4::SUB_LNAM:
                reader.get(mHairLength);
                break;
            case ESM4::SUB_HCLR:
            {
                reader.get(mHairColour.red);
                reader.get(mHairColour.green);
                reader.get(mHairColour.blue);
                reader.get(mHairColour.custom);

                break;
            }
            case ESM4::SUB_TPLT:
                reader.getFormId(mBaseTemplate);
                break;
            case ESM4::SUB_FGGS:
            {
                mSymShapeModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymShapeModeCoefficients.at(i));

                break;
            }
            case ESM4::SUB_FGGA:
            {
                mAsymShapeModeCoefficients.resize(30);
                for (std::size_t i = 0; i < 30; ++i)
                    reader.get(mAsymShapeModeCoefficients.at(i));

                break;
            }
            case ESM4::SUB_FGTS:
            {
                mSymTextureModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymTextureModeCoefficients.at(i));

                break;
            }
            case ESM4::SUB_FNAM:
            {
                reader.get(mFgRace);
                // std::cout << "race " << mEditorId << " " << mRace << std::endl; // FIXME
                // std::cout << "fg race " << mEditorId << " " << mFgRace << std::endl; // FIXME
                break;
            }
            case ESM4::SUB_PNAM: // FO3/FONV/TES5
                reader.getFormId(mHeadParts.emplace_back());
                break;
            case ESM4::SUB_HCLF: // TES5 hair colour
            {
                reader.getFormId(mHairColourId);

                break;
            }
            case ESM4::SUB_BCLF:
            {
                reader.getFormId(mBeardColourId);
                break;
            }
            case ESM4::SUB_COCT: // TES5
            {
                std::uint32_t count;
                reader.get(count);

                break;
            }
            case ESM4::SUB_DOFT:
                reader.getFormId(mDefaultOutfit);
                break;
            case ESM4::SUB_SOFT:
                reader.getFormId(mSleepOutfit);
                break;
            case ESM4::SUB_DPLT:
                reader.getFormId(mDefaultPkg);
                break; // AI package list
            case ESM4::SUB_DAMC: // Destructible
            case ESM4::SUB_DEST:
            case ESM4::SUB_DMDC:
            case ESM4::SUB_DMDL:
            case ESM4::SUB_DMDT:
            case ESM4::SUB_DMDS:
            case ESM4::SUB_DSTA:
            case ESM4::SUB_DSTD:
            case ESM4::SUB_DSTF: // Destructible end
            case ESM4::SUB_NAM6: // height mult
            case ESM4::SUB_NAM7: // weight mult
            case ESM4::SUB_ATKR:
            case ESM4::SUB_CRIF:
            case ESM4::SUB_CSDT:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_ECOR:
            case ESM4::SUB_ANAM:
            case ESM4::SUB_ATKD:
            case ESM4::SUB_ATKE:
            case ESM4::SUB_FTST:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM5:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_NAMA:
            case ESM4::SUB_OBND:
            case ESM4::SUB_PRKR:
            case ESM4::SUB_PRKZ:
            case ESM4::SUB_QNAM:
            case ESM4::SUB_SPCT:
            case ESM4::SUB_TIAS:
            case ESM4::SUB_TINC:
            case ESM4::SUB_TINI:
            case ESM4::SUB_TINV:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_VTCK:
            case ESM4::SUB_GNAM:
            case ESM4::SUB_SHRT:
            case ESM4::SUB_SPOR:
            case ESM4::SUB_EAMT: // FO3
            case ESM4::SUB_NAM4: // FO3
            case ESM4::SUB_COED: // FO3
            case ESM4::SUB_APPR: // FO4
            case ESM4::SUB_ATKS: // FO4
            case ESM4::SUB_ATKT: // FO4
            case ESM4::SUB_ATKW: // FO4
            case ESM4::SUB_ATTX: // FO4
            case ESM4::SUB_FTYP: // FO4
            case ESM4::SUB_LTPT: // FO4
            case ESM4::SUB_LTPC: // FO4
            case ESM4::SUB_MWGT: // FO4
            case ESM4::SUB_NTRM: // FO4
            case ESM4::SUB_PFRN: // FO4
            case ESM4::SUB_PRPS: // FO4
            case ESM4::SUB_PTRN: // FO4
            case ESM4::SUB_STCP: // FO4
            case ESM4::SUB_TETI: // FO4
            case ESM4::SUB_TEND: // FO4
            case ESM4::SUB_TPTA: // FO4
            case ESM4::SUB_OBTE: // FO4 object template start
            case ESM4::SUB_OBTF: //
            case ESM4::SUB_OBTS: //
            case ESM4::SUB_STOP: // FO4 object template end
            case ESM4::SUB_OCOR: // FO4 new package lists start
            case ESM4::SUB_GWOR: //
            case ESM4::SUB_FCPL: //
            case ESM4::SUB_RCLR: // FO4 new package lists end
            case ESM4::SUB_CS2D: // FO4 actor sound subrecords
            case ESM4::SUB_CS2E: //
            case ESM4::SUB_CS2F: //
            case ESM4::SUB_CS2H: //
            case ESM4::SUB_CS2K: // FO4 actor sound subrecords end
            case ESM4::SUB_MSDK: // FO4 morph subrecords start
            case ESM4::SUB_MSDV: //
            case ESM4::SUB_MRSV: //
            case ESM4::SUB_FMRI: //
            case ESM4::SUB_FMRS: //
            case ESM4::SUB_FMIN: // FO4 morph subrecords end
                reader.skipSubRecordData();
                break;
            default:
                throw std::runtime_error("ESM4::NPC_::load - Unknown subrecord " + ESM::printName(subHdr.typeId));
        }
    }
}

// void ESM4::Npc::save(ESM4::Writer& writer) const
//{
// }

// void ESM4::Npc::blank()
//{
// }
