//
// Created by jun on 2016/6/20.
//

#ifndef OVR_SDK_MOBILE_ANIMATIONPATH_H
#define OVR_SDK_MOBILE_ANIMATIONPATH_H

#include "Kernel/OVR_Math.h"
#include <map>

namespace OvrTemplateApp {
    class AnimationPath {
    public:
        AnimationPath():_loopMode(LOOP) {}
        static void slerp(OVR::Quatf &src, float t, const OVR::Quatf& from, const OVR::Quatf& to);

        class ControlPoint
        {
        public:
            ControlPoint():
                    _scale(1.0,1.0,1.0) {}

            ControlPoint(const OVR::Vector3f& position):
                    _position(position),
                    _rotation(),
                    _scale(1.0,1.0,1.0) {}

            ControlPoint(const OVR::Vector3f& position, const OVR::Quatf& rotation):
                    _position(position),
                    _rotation(rotation),
                    _scale(1.0,1.0,1.0) {}

            ControlPoint(const OVR::Vector3f& position, const OVR::Quatf& rotation, const OVR::Vector3f& scale):
                    _position(position),
                    _rotation(rotation),
                    _scale(scale) {}

            void setPosition(const OVR::Vector3f& position) { _position = position; }
            const OVR::Vector3f& getPosition() const { return _position; }

            void setRotation(const OVR::Quatf& rotation) { _rotation = rotation; }
            const OVR::Quatf& getRotation() const { return _rotation; }

            void setScale(const OVR::Vector3f& scale) { _scale = scale; }
            const OVR::Vector3f& getScale() const { return _scale; }

            inline void interpolate(float ratio,const ControlPoint& first, const ControlPoint& second)
            {
                float one_minus_ratio = 1.0f-ratio;
                _position = first._position*one_minus_ratio + second._position*ratio;
               // _rotation.slerp(ratio,first._rotation,second._rotation);
                AnimationPath::slerp(_rotation, ratio, first._rotation, second._rotation);
                _scale = first._scale*one_minus_ratio + second._scale*ratio;
            }


//
//            inline void getMatrix(OVR::Matrixf& matrix) const
//            {
//                matrix.makeRotate(_rotation);
//                matrix.preMultScale(_scale);
//                matrix.postMultTranslate(_position);
//            }

            inline void getMatrix(OVR::Matrix4f& matrix) const
            {
//                matrix.makeRotate(_rotation);
//                matrix.preMultScale(_scale);
//                matrix.postMultTranslate(_position); Matrix4f boxMatrix(boxPos);
                matrix.SetIdentity();
                matrix.SetTranslation(_position);
                //matrix *= OVR::Matrix4d(OVR::Posed(_position)).SetTranslation();
                matrix *= OVR::Matrix4f(_rotation);
                matrix *= OVR::Matrix4f::Scaling(_scale);
            }


        protected:

            OVR::Vector3f _position;
            OVR::Quatf _rotation;
            OVR::Vector3f _scale;

        };

        /** Given a specific time, return the local ControlPoint frame for a point. */
        virtual bool getInterpolatedControlPoint(double time,ControlPoint& controlPoint) const;

        /** Insert a control point into the AnimationPath.*/
        void insert(double time,const ControlPoint& controlPoint);

        double getFirstTime() const { if (!_timeControlPointMap.empty()) return _timeControlPointMap.begin()->first; else return 0.0;}
        double getLastTime() const { if (!_timeControlPointMap.empty()) return _timeControlPointMap.rbegin()->first; else return 0.0;}
        double getPeriod() const { return getLastTime()-getFirstTime();}

        bool empty() const { return _timeControlPointMap.empty(); }
        void clear(){ _timeControlPointMap.clear();}
        enum LoopMode
        {
            SWING,
            LOOP,
            NO_LOOPING
        };

        void setLoopMode(LoopMode lm) { _loopMode = lm; }

        LoopMode getLoopMode() const { return _loopMode; }
        typedef std::map<double, ControlPoint> TimeControlPointMap;

    protected:

        TimeControlPointMap _timeControlPointMap;
        LoopMode            _loopMode;

    };

}

#endif //OVR_SDK_MOBILE_ANIMATIONPATH_H
