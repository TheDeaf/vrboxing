//
// Created by jun on 2016/6/16.
//

#ifndef OVR_SDK_MOBILE_ANIMATIONMANAGER_H
#define OVR_SDK_MOBILE_ANIMATIONMANAGER_H

#include "SceneView.h"

namespace OvrTemplateApp {

    class AnimationManager {

    public:
        AnimationManager(OVR::ModelInScene *pModelInScene);

        //begin animation
        bool BeginAnimation(double dStartTime, double dSpeed);

        // update before frame
        // if first end animation return true
        bool Update(const OVR::VrFrame &vrFrame);

        // get is animationing
        bool IsAnimationing() const {return m_bAnimationing;}

    private:
        //
        bool m_bAnimationing;
        //
        OVR::ModelInScene *m_pModelInScene;
        //start time seconds
        double m_dStartTime;
        // animation time
        double m_dSpeed;

    };
}


#endif //OVR_SDK_MOBILE_ANIMATIONMANAGER_H
