#include "loadclot.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Clothing::sRecordId = REC_CLOT;

    void Clothing::load(ESMReader &esm, bool &isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        mParts.mParts.clear();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case ESM::SREC_NAME:
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case ESM::fourCC("MODL"):
                    mModel = esm.getHString();
                    break;
                case ESM::fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case ESM::fourCC("CTDT"):
                    esm.getHTSized<12>(mData);
                    hasData = true;
                    break;
                case ESM::fourCC("SCRI"):
                    mScript = esm.getHString();
                    break;
                case ESM::fourCC("ITEX"):
                    mIcon = esm.getHString();
                    break;
                case ESM::fourCC("ENAM"):
                    mEnchant = esm.getHString();
                    break;
                case ESM::fourCC("INDX"):
                    mParts.add(esm);
                    break;
                case ESM::SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing CTDT subrecord");
    }

    void Clothing::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNCString("MODL", mModel);
        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CTDT", mData, 12);

        esm.writeHNOCString("SCRI", mScript);
        esm.writeHNOCString("ITEX", mIcon);

        mParts.save(esm);

        esm.writeHNOCString("ENAM", mEnchant);
    }

    void Clothing::blank()
    {
        mData.mType = 0;
        mData.mWeight = 0;
        mData.mValue = 0;
        mData.mEnchant = 0;
        mParts.mParts.clear();
        mName.clear();
        mModel.clear();
        mIcon.clear();
        mEnchant.clear();
        mScript.clear();
    }
}
