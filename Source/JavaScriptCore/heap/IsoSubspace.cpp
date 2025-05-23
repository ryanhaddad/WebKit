/*
 * Copyright (C) 2017-2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "IsoSubspace.h"

#include "FastMallocAlignedMemoryAllocator.h"
#include "IsoCellSetInlines.h"
#include "JSCellInlines.h"
#include "MarkedSpaceInlines.h"
#include <wtf/TZoneMallocInlines.h>

namespace JSC {

WTF_MAKE_TZONE_ALLOCATED_IMPL(IsoSubspace);

IsoSubspace::IsoSubspace(CString name, JSC::Heap& heap, const HeapCellType& heapCellType, size_t size, uint8_t numberOfLowerTierPreciseCells, std::unique_ptr<AlignedMemoryAllocator>&& allocator)
    : Subspace(SubspaceKind::IsoSubspace, name, heap)
    , m_directory(WTF::roundUpToMultipleOf<MarkedBlock::atomSize>(size))
    , m_allocator(allocator ? WTFMove(allocator) : makeUnique<FastMallocAlignedMemoryAllocator>())
{
    m_remainingLowerTierPreciseCount = numberOfLowerTierPreciseCells;
    ASSERT(WTF::roundUpToMultipleOf<MarkedBlock::atomSize>(size) == cellSize());
    ASSERT(m_remainingLowerTierPreciseCount <= MarkedBlock::maxNumberOfLowerTierPreciseCells);

    initialize(heapCellType, m_allocator.get());

    Locker locker { m_space.directoryLock() };
    m_directory.setSubspace(this);
    m_space.addBlockDirectory(locker, &m_directory);
    m_alignedMemoryAllocator->registerDirectory(heap, &m_directory);
    m_firstDirectory = &m_directory;
}

IsoSubspace::~IsoSubspace() = default;

void IsoSubspace::didResizeBits(unsigned blockIndex)
{
    m_cellSets.forEach(
        [&] (IsoCellSet* set) {
            set->didResizeBits(blockIndex);
        });
}

void IsoSubspace::didRemoveBlock(unsigned blockIndex)
{
    m_cellSets.forEach(
        [&] (IsoCellSet* set) {
            set->didRemoveBlock(blockIndex);
        });
}

void IsoSubspace::didBeginSweepingToFreeList(MarkedBlock::Handle* block)
{
    m_cellSets.forEach(
        [&] (IsoCellSet* set) {
            set->sweepToFreeList(block);
        });
}

void* IsoSubspace::tryAllocateLowerTierPrecise(size_t size)
{
    auto revive = [&] (PreciseAllocation* allocation) {
        // Lower-tier cells never report capacity. This is intentional since it will not be freed until VM dies.
        // Whether we will do GC or not does not affect on the used memory by lower-tier cells. So we should not
        // count them in capacity since it is not interesting to decide whether we should do GC.
        m_preciseAllocations.append(allocation);
        m_space.registerPreciseAllocation(allocation, /* isNewAllocation */ false);
        ASSERT(allocation->indexInSpace() == m_space.m_preciseAllocations.size() - 1);
        return allocation->cell();
    };

    ASSERT_WITH_MESSAGE(cellSize() == size, "non-preciseOnly IsoSubspaces shouldn't have variable size");
    if (!m_lowerTierPreciseFreeList.isEmpty()) {
        PreciseAllocation* allocation = &*m_lowerTierPreciseFreeList.begin();
        allocation->remove();
        return revive(allocation);
    }
    if (m_remainingLowerTierPreciseCount) {
        PreciseAllocation* allocation = PreciseAllocation::tryCreateForLowerTierPrecise(m_space.heap(), size, this, --m_remainingLowerTierPreciseCount);
        if (allocation)
            return revive(allocation);
    }
    return nullptr;
}

void IsoSubspace::sweepLowerTierPreciseCell(PreciseAllocation* preciseAllocation)
{
    preciseAllocation = preciseAllocation->reuseForLowerTierPrecise();
    m_lowerTierPreciseFreeList.append(preciseAllocation);
}

void IsoSubspace::destroyLowerTierPreciseFreeList()
{
    m_lowerTierPreciseFreeList.forEach([&](PreciseAllocation* allocation) {
        allocation->destroy();
    });
}

namespace GCClient {

IsoSubspace::IsoSubspace(JSC::IsoSubspace& server)
    : m_localAllocator(&server.m_directory)
{
}

} // namespace GCClient

} // namespace JSC

