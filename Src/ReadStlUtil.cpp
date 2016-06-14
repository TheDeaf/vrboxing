//
// Created by jun on 2016/6/13.
//

#include "ReadStlUtil.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include "Kernel/OVR_String_Utils.h"

using namespace OVR;


struct StlHeader
{
    char text[80];
    unsigned int numFacets;
};
const unsigned int sizeof_StlHeader = 84;

struct StlVector
{
    float x, y, z;
};
struct StlFacet
{
    StlVector normal;
    StlVector vertex[3];
    unsigned short color;
};
const unsigned int sizeof_StlFacet = 50;

const unsigned short StlHasColor = 0x8000;
const unsigned short StlColorSize = 0x1f;        // 5 bit
const float StlColorDepth = float(StlColorSize); // 2^5 - 1


class ReaderObject
{
public:
    ReaderObject(bool generateNormals = true):
            _generateNormal(generateNormals),
            _numFacets(0)
    {
    }

    virtual ~ReaderObject()
    {
    }

    enum ReadResult
    {
        ReadSuccess,
        ReadError,
        ReadEOF
    };

    virtual ReadResult read(FILE *fp) = 0;
    bool isEmpty()
    {
        return _numFacets == 0;
    }

    std::string& getName()
    {
        return _solidName;
    }

public:
    bool _generateNormal;
    unsigned int _numFacets;

    std::string _solidName;

    Array< Vector3f > _vertex;
    Array< Vector3f > _normal;
    Array< Vector4f > _color;
    Bounds3f _bound;

    void clear()
    {
        _solidName = "";
        _numFacets = 0;
        _vertex.Clear();
        _normal.Clear();
        _color.Clear();
        _bound.Clear();
    }
};

class BinaryReaderObject : public ReaderObject
{
public:
    BinaryReaderObject(unsigned int expectNumFacets, bool generateNormals = true)
            : ReaderObject(generateNormals),
              _expectNumFacets(expectNumFacets)
    {
    }

    ReadResult read(FILE *fp);

protected:
    unsigned int _expectNumFacets;
};
ReaderObject::ReadResult BinaryReaderObject::read(FILE* fp)
{
    if (ReaderObject::isEmpty())
    {
        ReaderObject::clear();
    }

    _numFacets = _expectNumFacets;

    // seek to beginning of facets
    ::fseek(fp, sizeof_StlHeader, SEEK_SET);

    StlFacet facet;
    for (unsigned int i = 0; i < _expectNumFacets; ++i)
    {
        if (::fread((void*) &facet, sizeof_StlFacet, 1, fp) != 1)
        {
            //OVR::Capture ::Logf(OVR::Capture::Log_Info, "ReaderWriterSTL::readStlBinary: Failed to read facet: %ud", i);
            return ReadError;
        }

//        // vertices
//        if (!_vertex.valid())
//            _vertex = new osg::Vec3Array;

        Vector3f v0(facet.vertex[0].x, facet.vertex[0].y, facet.vertex[0].z);
        Vector3f v1(facet.vertex[1].x, facet.vertex[1].y, facet.vertex[1].z);
        Vector3f v2(facet.vertex[2].x, facet.vertex[2].y, facet.vertex[2].z);
        _vertex.PushBack(v0);
        _vertex.PushBack(v1);
        _vertex.PushBack(v2);
        _bound.AddPoint(v0);
        _bound.AddPoint(v1);
        _bound.AddPoint(v2);
        //OVR::Capture::Logf(OVR::Capture::Log_Info, "pt1: %s", StringUtils::ToString(v0).ToCStr());
        //OVR::Capture::Logf(OVR::Capture::Log_Info, "pt2: %s", StringUtils::ToString(v1).ToCStr());
        //OVR::Capture::Logf(OVR::Capture::Log_Info, "pt3: %s", StringUtils::ToString(v2).ToCStr());

        // per-facet normal
        Vector3f normal;
        if (_generateNormal)
        {
            Vector3f d01 = v1 - v0;
            Vector3f d02 = v2 - v0;
            normal = d01.Cross(d02);
            normal.Normalize();
        }
        else
        {
            normal.Set(facet.normal.x, facet.normal.y, facet.normal.z);
        }

//        if (!_normal.valid())
//            _normal = new osg::Vec3Array;
        _normal.PushBack(normal);

        /*
         * color extension
         * RGB555 with most-significat bit indicating if color is present
         */
        if (facet.color & StlHasColor)
        {
//            if (!_color.valid())
//            {
//                _color = new osg::Vec4Array;
//            }
            float r = ((facet.color >> 10) & StlColorSize) / StlColorDepth;
            float g = ((facet.color >> 5) & StlColorSize) / StlColorDepth;
            float b = (facet.color & StlColorSize) / StlColorDepth;
            _color.PushBack(Vector4f(r, g, b, 1.0f));
        }
    }

    return ReadEOF;
}

bool CReadStlUtil::ReadStlNode(const char * fileName, VertexAttribs &attribs, Array< TriangleIndex > &indices, Bounds3f &cullingBounds)
{

    //OVR::Capture::Logf(OVR::Capture::Log_Info, "ReadFbxNode: %s", fileName);

// stl is easy
    // determine ASCII vs. binary mode
    FILE* fp = fopen(fileName, "rb");
    if (!fp)
    {
        //OVR::Capture::Logf(OVR::Capture::Log_Info, "ReadFbxNode: %s", fileName);
        return false;
    }

    // assumes "unsigned int" is 4 bytes...
    StlHeader header;
    if (fread((void*) &header, sizeof(header), 1, fp) != 1)
    {
        fclose(fp);
        return false;
    }


    struct stat stb;
    if (fstat(fileno(fp), &stb) < 0)
    {
        //OVR::Capture::Log(OVR::Capture::Log_Info, "ReaderWriterSTL::readNode: Unable to stat:");
        fclose(fp);
        return false;
    }

    bool isBinary = false;
    // calculate expected file length from number of facets
    unsigned int expectFacets = header.numFacets;
//    if (osg::getCpuByteOrder() == osg::BigEndian)
//    {
//        osg::swapBytes4((char*) &expectFacets);
//    }
    off_t expectLen = sizeof_StlHeader + expectFacets * sizeof_StlFacet;
    if (stb.st_size == expectLen)
    {
        isBinary = true;
    }
    else if (strstr(header.text, "solid") != 0)
    {
        isBinary = false;
    }
    else
    {
       // OVR::Capture::Log(OVR::Capture::Log_Info, "ReaderWriterSTL::readNode unable to determine file format");
        fclose(fp);
        return false;
    }

    if (!isBinary)
    {
        fclose(fp);
       // OVR::Capture::Log(OVR::Capture::Log_Info, "is not binary");
        fp = fopen(fileName, "r");
    }
    // read
    rewind(fp);
    BinaryReaderObject readerPtr(expectFacets);

    if (!isBinary)
    {

        fclose(fp);
        return true;
    }

    while (1)
    {
        ReaderObject::ReadResult result;

        if ((result = readerPtr.read(fp)) == ReaderObject::ReadError)
        {
            fclose(fp);
            //OVR::Capture::Log(OVR::Capture::Log_Info, "reder readError");
            return false;
        }

        if (!readerPtr.isEmpty())
        {
           // OVR::Capture::Log(OVR::Capture::Log_Info, "reder is not empty");
            attribs.position.Append(readerPtr._vertex);
            attribs.color.Append(readerPtr._color);
            cullingBounds = Bounds3f::Union(cullingBounds, readerPtr._bound);
        }

        if (result == ReaderObject::ReadEOF)
            break;
    }
    for(unsigned short i = 0; i < attribs.position.GetSize(); i++)
    {
        indices.PushBack(i);
    }
    //OVR::Capture::Logf(OVR::Capture::Log_Info, "position size: %d", attribs.position.GetSize());
    //OVR::Capture::Logf(OVR::Capture::Log_Info, "color size: %d", attribs.color.GetSize());
    fclose(fp);
    return true;
}