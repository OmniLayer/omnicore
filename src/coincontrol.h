// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COINCONTROL_H
#define BITCOIN_COINCONTROL_H

#include "primitives/transaction.h"

/** Coin Control Features. */
class CCoinControl
{
public:
    CTxDestination destChange;

    CCoinControl()
    {
        SetNull();
        is_omni = false;
    }

    void SetNull()
    {
        destChange = CNoDestination();
        setSelected.clear();
    }

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    COutPoint GetFirst() const
    {
        return *setSelected.begin();
    }

    bool IsSelected(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        for (std::vector<COutPoint>::const_iterator x = setSelected.begin(); x != setSelected.end(); x++)
            if (*x == outpt)
                return true;
        return false;
    }

    void Select(const COutPoint& output)
    {
        if (!IsSelected(output.hash, output.n))
            setSelected.push_back(output);
    }

    void UnSelect(const COutPoint& output)
    {
        for (std::vector<COutPoint>::iterator x = setSelected.begin(); x != setSelected.end(); x++)
            if (*x == output)
            {
                setSelected.erase(x);
                break;
            }
    }

    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints)
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

    bool IsOmni() const
    {
        return is_omni;
    }

    void SetOmni(bool omni = true)
    {
        is_omni = omni;;
    }

private:
    std::vector<COutPoint> setSelected;
    bool is_omni;
};

#endif // BITCOIN_COINCONTROL_H
