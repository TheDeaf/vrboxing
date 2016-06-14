//
// Created by jun on 2016/6/13.
//

#ifndef OVR_SDK_MOBILE_CREADSTLUTIL_H
#define OVR_SDK_MOBILE_CREADSTLUTIL_H

#include "App.h"
#include "SceneView.h"
#include <memory>
#include "GuiSys.h"
#include "OvrApp.h"
#include "GuiSys.h"
#include "OVR_Locale.h"

class CReadStlUtil {

public:
    // des: read stl node
    // fileName;
    // attribs:  verts color(no)
    // indices: tri index
    // cullingBounds: bounds
    // author: mj; 2016-06-13
    static bool ReadStlNode(const char * fileName, OVR::VertexAttribs &attribs, OVR::Array< OVR::TriangleIndex > &indices
            , OVR::Bounds3f &cullingBounds);
};


#endif //OVR_SDK_MOBILE_CREADSTLUTIL_H
