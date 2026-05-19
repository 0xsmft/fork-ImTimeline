#pragma once

class IDGenerator
{
public:
    int GetUniqueID()
    {
        mID++;
        return mID;
    }
    private:
    s32 mID = 0;
};
