#include "stdafx.h"
#include "StreamingImageSequence/ImageCatalog.h"

namespace StreamingImageSequencePlugin {

ImageCatalog::ImageCatalog() {
    for (uint32_t imageType = 0; imageType < MAX_CRITICAL_SECTION_TYPE_IMAGES; ++imageType) {
        m_imageCollection[imageType].Init(static_cast<CriticalSectionType>(imageType), &m_memAllocator);
    }

}

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

//Thread-safe
void ImageCatalog::Reset() {
    for (uint32_t imageType = 0; imageType < MAX_CRITICAL_SECTION_TYPE_IMAGES; ++imageType) {
        m_imageCollection[imageType].ResetAll();
    }
}

//Thread-safe
void ImageCatalog::ResetOrder() {

    for (uint32_t imageType = 0; imageType < MAX_CRITICAL_SECTION_TYPE_IMAGES; ++imageType) {
        m_imageCollection[imageType].ResetOrder();
    }
}

//----------------------------------------------------------------------------------------------------------------------



} //end namespace
