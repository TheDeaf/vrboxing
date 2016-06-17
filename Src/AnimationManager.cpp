//
// Created by jun on 2016/6/16.
//

#include "AnimationManager.h"
#include <OVR_Capture.h>


using namespace OVR;

namespace OvrTemplateApp
{

    AnimationManager::AnimationManager(OVR::ModelInScene *pModelInScene)
    :m_bAnimationing(false)
    ,m_pModelInScene(pModelInScene)
    ,m_dStartTime(0.0)
    ,m_dSpeed(0.0)
    {

    }
    bool AnimationManager::BeginAnimation(double dStartTime, double dSpeed, float fEyeYaw)
    {
        if (m_pModelInScene == NULL)
            return false;
        if (dSpeed < 0.00001)
            return false;
        OVR::Capture::Log(OVR::Capture::Log_Info, "beginAnimation");
        m_bAnimationing = true;
        m_pModelInScene->State.DontRenderForClientUid = 0;
        m_dStartTime = dStartTime;
        m_dSpeed = dSpeed;
        m_fEyeYaw = fEyeYaw;

        return true;
    }
    bool AnimationManager::Update(const OVR::VrFrame &vrFrame)
    {
        if (m_pModelInScene == NULL || !m_bAnimationing)
            return false;
        double dTimeNow = vrapi_GetTimeInSeconds();
        double dAnimationTime = dTimeNow - m_dStartTime;
        if (dAnimationTime < 1.0E-5)
            dAnimationTime = 0.0;
        Vector3f ptS = Matrix4f::RotationY(m_fEyeYaw).Transform(Vector3f(0.25f, 1.5f, 0.1f));//(0.4f, 1.5f, 0.1f);
        Vector3f ptE = Matrix4f::RotationY(m_fEyeYaw).Transform(Vector3f(0.25f, 1.5f, -1.4f));//(0.4f, 1.5f, -1.0f);
        Vector3f vecMove = ptE - ptS;
        float fLength = vecMove.Length();
        double dMoveAllTime = fLength / m_dSpeed;
        if(dAnimationTime > dMoveAllTime)
        {
            m_bAnimationing = false;
            m_pModelInScene->State.DontRenderForClientUid = -1;
            OVR::Capture::Log(OVR::Capture::Log_Info, "end Animation");
            return true;
        }
        vecMove.Normalize();

        Vector3f ptNow =ptS + vecMove * dAnimationTime * m_dSpeed;

        Posef boxPos;
        boxPos.Position = ptNow;
        //boxPos.Orientation = Quatf(Matrix4f::RotationZ(-Mathf::PiOver2));
        Matrix4f boxMatrix(boxPos);

        boxMatrix *= Matrix4f::Scaling(0.001);
        boxMatrix *= Matrix4f::RotationY(m_fEyeYaw);
        boxMatrix *= Matrix4f::RotationY(Mathf::PiOver2);
        boxMatrix *= Matrix4f::RotationZ(-Mathf::PiOver2);
        m_pModelInScene->State.modelMatrix = boxMatrix;
        return false;
    }

}