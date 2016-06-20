//
// Created by jun on 2016/6/20.
//

#include "AnimationPath.h"

using namespace OVR;

namespace OvrTemplateApp{
    void AnimationPath::slerp(Quatf &src, float t, const Quatf &from,
                              const Quatf &to)
    {
        const float epsilon = 0.00001;
        float omega = 0.0f, cosomega = 0.0f, sinomega = 0.0f, scale_from = 0.0f, scale_to = 0.0f ;

        Quatf quatTo(to);
        // this is a dot product

        cosomega = Vector4f(from.x, from.y, from.z, from.w).Dot(Vector4f(to.x, to.y, to.z, to.w));//from.asVec4() * to.asVec4();

        if ( cosomega <0.0f )
        {
            cosomega = -cosomega;
            quatTo = to.Inverted();
        }

        if( (1.0f - cosomega) > epsilon )
        {
            omega= acos(cosomega) ;  // 0 <= omega <= Pi (see man acos)
            sinomega = sin(omega) ;  // this sinomega should always be +ve so
            // could try sinomega=sqrt(1-cosomega*cosomega) to avoid a sin()?
            scale_from = sin((1.0f-t)*omega)/sinomega ;
            scale_to = sin(t*omega)/sinomega ;
        }
        else
        {
            /* --------------------------------------------------
               The ends of the vectors are very close
               we can use simple linear interpolation - no need
               to worry about the "spherical" interpolation
               -------------------------------------------------- */
            scale_from = 1.0f - t ;
            scale_to = t ;
        }

        src = (from*scale_from) + (quatTo*scale_to);

    }

    void AnimationPath::insert(double time,const ControlPoint& controlPoint)
    {
        _timeControlPointMap[time] = controlPoint;
    }

    bool AnimationPath::getInterpolatedControlPoint(double time,ControlPoint& controlPoint) const
    {
        if (_timeControlPointMap.empty()) return false;

        switch(_loopMode)
        {
            case(SWING):
            {
                double modulated_time = (time - getFirstTime())/(getPeriod()*2.0);
                double fraction_part = modulated_time - floor(modulated_time);
                if (fraction_part>0.5) fraction_part = 1.0-fraction_part;

                time = getFirstTime()+(fraction_part*2.0) * getPeriod();
                break;
            }
            case(LOOP):
            {
                double modulated_time = (time - getFirstTime())/getPeriod();
                double fraction_part = modulated_time - floor(modulated_time);
                time = getFirstTime()+fraction_part * getPeriod();
                break;
            }
            case(NO_LOOPING):
                // no need to modulate the time.
                break;
        }



        TimeControlPointMap::const_iterator second = _timeControlPointMap.lower_bound(time);
        if (second==_timeControlPointMap.begin())
        {
            controlPoint = second->second;
        }
        else if (second!=_timeControlPointMap.end())
        {
            TimeControlPointMap::const_iterator first = second;
            --first;

            // we have both a lower bound and the next item.

            // delta_time = second.time - first.time
            double delta_time = second->first - first->first;

            if (delta_time==0.0f)
                controlPoint = first->second;
            else
            {
                controlPoint.interpolate((time - first->first)/delta_time,
                                         first->second,
                                         second->second);
            }
        }
        else // (second==_timeControlPointMap.end())
        {
            controlPoint = _timeControlPointMap.rbegin()->second;
        }
        return true;
    }

}