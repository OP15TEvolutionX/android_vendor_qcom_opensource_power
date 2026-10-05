// Copyright (C) 2026 Evolution X
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mutex>

// Independent, idempotent locks: disabling one mode must not release the other.
// Resources live in the device powerhint.xml; no direct sysfs writes are made.
class PowerSavingModes {
  public:
    using Acquire = int (*)(int, int, int);
    using Release = void (*)(int);
    PowerSavingModes(Acquire acquire, Release release) : mAcquire(acquire), mRelease(release) {}
    ~PowerSavingModes() { setLowPower(false); setDeviceIdle(false); }
    bool setLowPower(bool enabled) { return set(mLowPowerHandle, -1, enabled); }
    bool setDeviceIdle(bool enabled) { return set(mDeviceIdleHandle, 1, enabled); }

  private:
    bool set(int& handle, int type, bool enabled) {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!enabled) {
            if (handle > 0) mRelease(handle);
            handle = 0;
            return true;
        }
        if (handle > 0) return true;
        // AOSP LOW_POWER (0x1205); type 1 is the separate DEVICE_IDLE profile.
        int acquired = mAcquire(0x1205, 0, type);
        if (acquired <= 0) return false;
        handle = acquired;
        return true;
    }
    Acquire mAcquire;
    Release mRelease;
    std::mutex mMutex;
    int mLowPowerHandle = 0;
    int mDeviceIdleHandle = 0;
};
