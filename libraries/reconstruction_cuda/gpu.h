/***************************************************************************
 *
 * Authors:    David Strelak (davidstrelak@gmail.com)
 *
 * Unidad de  Bioinformatica of Centro Nacional de Biotecnologia , CSIC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307  USA
 *
 *  All comments concerning this program package may be sent to the
 *  e-mail address 'xmipp@cnb.csic.es'
 ***************************************************************************/

#ifndef LIBRARIES_RECONSTRUCTION_CUDA_GPU_H_
#define LIBRARIES_RECONSTRUCTION_CUDA_GPU_H_

#include <cstddef>
#include <assert.h>
#include "core/xmipp_error.h"

class GPU {
public:
    explicit GPU(int device = 0, int stream = 0):
        m_lastFreeBytes(0), m_totalBytes(0), m_device(device),
        m_streamId(stream), m_stream(nullptr),
        m_isSet(false) {};

    ~GPU();

    inline int device() const {
        return m_device;
    }

    inline void* stream() const {
        check();
        return m_stream;
    }

    inline int streamId() const {
        return m_streamId;
    }

    inline size_t lastFreeBytes() const {
        check();
        return m_lastFreeBytes;
    }

    inline size_t totalBytes() const {
        check();
        return m_totalBytes;
    }

    inline size_t lastUsedBytes() const {
        check();
        return m_totalBytes - m_lastFreeBytes;
    }

    void updateMemoryInfo();

    void peekLastError() const;

    void pinMemory(void *h_mem, size_t bytes, unsigned int flags = 0) const;

    void unpinMemory(void *h_mem) const;

    void set();

    void synchStream() const;

    void synch() const;

    inline std::string getUUID() const {
        check();
        return m_uuid;
    }

    inline bool isSet() const {
        return m_isSet;
    }

    static inline int getDeviceCount();

private:
    size_t m_totalBytes;
    size_t m_lastFreeBytes;
    int m_device;
    int m_streamId;
    void* m_stream;
    bool m_isSet;

    std::string m_uuid;

    inline void check() const {
        if ( ! m_isSet) {
            REPORT_ERROR(ERR_LOGIC_ERROR, "You have to set() this GPU before using it");
        };
    }

    void obtainUUID();
};

#endif /* LIBRARIES_RECONSTRUCTION_CUDA_GPU_H_ */