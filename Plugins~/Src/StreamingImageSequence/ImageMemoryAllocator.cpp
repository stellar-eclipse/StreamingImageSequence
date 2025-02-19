#include "StreamingImageSequence/ImageMemoryAllocator.h"

#include "CommonLib/MemoryUtility.h"

#include "LoaderConstants.h"

namespace StreamingImageSequencePlugin {

const uint64_t UNLIMITED_MEMORY = 0;
const uint64_t DEFAULT_MAX_MEMORY = UNLIMITED_MEMORY;

//----------------------------------------------------------------------------------------------------------------------

ImageMemoryAllocator::ImageMemoryAllocator() : m_usedMemory(0)
    , m_maxMemory(DEFAULT_MAX_MEMORY)
    , m_totalRAM(static_cast<float>(MemoryUtility::GetTotalRAM()))
    , m_inverseTotalRAM(1.0f / MemoryUtility::GetTotalRAM())
{
#ifdef MAX_IMAGE_MEMORY //overwrite for testing
    m_maxMemory = MAX_IMAGE_MEMORY;
#endif

}


ImageMemoryAllocator::~ImageMemoryAllocator() {
    ASSERT(0 == m_usedMemory);
}

//----------------------------------------------------------------------------------------------------------------------

bool ImageMemoryAllocator::Allocate(uint8_t ** rawDataPtr, const uint32_t w, const uint32_t h) {
    const size_t dataSize = CalculateMemSize(w, h);
    if (!IsMemoryAllocable(dataSize))
        return false;

    uint8_t*  buffer = static_cast<uint8_t*>(AllocateInternal(dataSize));
    if (nullptr == buffer) {
        return false;
    }

    *rawDataPtr = buffer;
    return true;

}

//return nullptr if not successful
void* ImageMemoryAllocator::Allocate(const size_t memSize, bool forceAllocate) {
    //only check with the internal memory condition if forceAllocate is false
    if (!forceAllocate && !IsMemoryAllocable(memSize))
        return nullptr;

    return AllocateInternal(memSize);
}

void* ImageMemoryAllocator::Reallocate(void* buffer, const size_t memSize, bool forceAllocate) {

    if (nullptr == buffer) {
        return Allocate(memSize, forceAllocate);
    }
    const auto allocatedBuffer = m_allocatedBuffers.find(buffer);
    if (m_allocatedBuffers.end() == allocatedBuffer ) {
        return nullptr;
    }

    const size_t prevSize = allocatedBuffer->second;

    void* newBuffer = Allocate(memSize, forceAllocate);
    if (nullptr == newBuffer)
        return nullptr;

    //[TODO-sin: 2020-10-15] min macro conflicts with std::min in Windows.
    //This can be avoided by defining NOMINMAX, but unfortunately the loader module in Windows is using GDI which requires
    //the windows Macro.
    //We are going to replace GDI with stb soon anyway, so for now, just use ifdef

#ifdef _WIN32
    std::memcpy(newBuffer, buffer, min(prevSize, memSize));
#else
    std::memcpy(newBuffer, buffer, std::min(prevSize, memSize));
#endif
    Deallocate(buffer);
    return newBuffer;
}
//----------------------------------------------------------------------------------------------------------------------

bool ImageMemoryAllocator::Deallocate(ImageData* imageData) {
    ASSERT(nullptr!=imageData);
    if (!Deallocate(imageData->RawData)) {
        return false;
    }

    *imageData = ImageData(nullptr, 0, 0, READ_STATUS_IDLE);
    return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool ImageMemoryAllocator::Deallocate(void* buffer) {

    if (nullptr == buffer) {
        return false;
    }
    const auto allocatedBuffer = m_allocatedBuffers.find(buffer);
    if (m_allocatedBuffers.end() == allocatedBuffer ) {
        return false;
    }

    const size_t memSize = allocatedBuffer->second;
    ASSERT(m_usedMemory >= memSize);
    m_allocatedBuffers.erase(buffer);
    DecUsedMem(memSize);
    free(buffer);

    return true;
}


//----------------------------------------------------------------------------------------------------------------------
bool ImageMemoryAllocator::IsMemoryAllocable(const size_t memSize) const {
    if (m_maxMemory != UNLIMITED_MEMORY && (m_usedMemory + memSize) > m_maxMemory)
        return false;

#if OSX
    //Mac (10.9+) compresses inactive memory automatically to free memory to be used by other application, and therefore
    //we can't directly use the amount of free RAM returned by the OS (the value is often near zero).
    //For now, we simply check if the number of used memory has exceeded the total RAM.
    if (m_usedMemory + memSize > m_totalRAM)
        return false;

#else
    const float MIN_AVAILABLE_RAM_RATIO = 0.1f;
    const float availableRAMAfterAllocateRatio  = (MemoryUtility::GetAvailableRAM() - memSize) * m_inverseTotalRAM;
                                      
    if ((availableRAMAfterAllocateRatio) <= MIN_AVAILABLE_RAM_RATIO)
        return false;
#endif

    return true;
}

//----------------------------------------------------------------------------------------------------------------------
void* ImageMemoryAllocator::AllocateInternal(const size_t memSize) {
    uint8_t*  buffer = static_cast<uint8_t*>(malloc(memSize));
    if (nullptr == buffer) {
        return nullptr;
    }

    //std::memset(buffer,0,memSize);
    m_allocatedBuffers[buffer] = memSize;
    IncUsedMem(memSize);
    return buffer;
}

//----------------------------------------------------------------------------------------------------------------------
void ImageMemoryAllocator::IncUsedMem(const uint64_t mem) {
    m_usedMemory += mem;
}

void ImageMemoryAllocator::DecUsedMem(const uint64_t mem) {
    m_usedMemory = (m_usedMemory >= mem) ? m_usedMemory - mem : 0;
}

size_t ImageMemoryAllocator::CalculateMemSize(const uint32_t w, const uint32_t h) {
    return w * h * LoaderConstants::NUM_BYTES_PER_TEXEL;
}
//----------------------------------------------------------------------------------------------------------------------


} //end namespace
