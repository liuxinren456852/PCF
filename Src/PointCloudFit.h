#ifndef _POINT_CLOUD_FIT_H_FILE_
#define _POINT_CLOUD_FIT_H_FILE_

#if 1
#define _RECON_DATA_ 1
#else
#define _SYN_DATA_ 1
#endif

#include "MeshDoc.h"
#include "GeometryObject.h"
#include "utility/flog.h"
#ifdef _USE_OPENMP_
#include <omp.h>
#endif

#include <QTime>

void doFailure();

#define PtAttri_GeoType "PerVerAttri_PtType"
typedef int PtType;
#define MaxModelNum 256
enum _PtType {
	Pt_Undefined  = 0x0000, // 00000
	Pt_Noise      = 0x0100, // 00001
	Pt_OnPlane    = 0x0200, // 00010
    Pt_OnCube     = 0x0600, // 00110
    Pt_OnCylinder = 0x0800, // 01000
    Pt_OnCone     = 0x1800  // 11000
};
int _ResetObjCode(_PtType type);
int _GetObjCode(_PtType type);

class PCFit
{
public:
	PCFit(const int nThread = 0);
	PCFit(const char *plyFilePath, const int nThread = 0);
	~PCFit();

	void printLogo();
	void printParams();

	void setThreadNum(const int nThread);
	void initMParams(const char *iniFile = 0);

	bool loadPly(const char *plyFilePath);
	bool savePly(const char *plyFilePath);
	void clearD();
	void inverseD();
	int deletePts(const int maskID);
	int keepPts(const int maskID);
	int recolorPts(const int maskID, const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a = 255);
	int deletePtType(const _PtType type);
	int keepPtType(const _PtType type);
	int recolorPtType(const _PtType type, const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a = 255);
	int resetType(const _PtType type);

	void autoColor();
    enum ProType {
        DoNothing                = 0x00,
        //---------------------
        OneStep_RemoveOutlier    = 0x01,
        OneStep_DetectCylinder   = 0x02,
        OneStep_DetectPlane      = 0x04,
        OneStep_DetectCube       = 0x08,
        //---------------------
        SpSteps_OnlyPlane        = 0x04,  // Without Remove Outlier Step
        SpSteps_ToPlane          = 0x06,  // Without Remove Outlier Step
        SpSteps_ToCube           = 0x0E,  // Without Remove Outlier Step
        Steps_OnlyPlane          = 0x05,
        Steps_ToPlane            = 0x07,
        Steps_ToCube             = 0x0F,
        Steps_All                = 0x0F,
        //---------------------
        
    };
	bool GEOFit(ProType proType = Steps_All, bool keepAttribute = true);
	ObjSet *getGEOObjSet() { return m_GEOObjSet; }
public:
	// All Thresholds
    // ��������-[�������ø�]
	int Threshold_NPts;                      // ���������С��[Threshold_NPts]�����д���
	// double RefA_Ratio;                       // ��λ���ȱ���

    // ȥ�����-[�������ø�]
	int DeNoise_MaxIteration;                // ȥ�������ֵ, Ĭ��100(<=0)
	int DeNoise_KNNNeighbors;                // ȥ��ʱKNN����ڸ���(KNN)
    int DeNoise_GrowNeighbors;               // ȥ��ʱKNN����ڸ���(��������)
	double DeNoise_DisRatioOfOutlier;        // ȥ����������ֵ

    // ƽ��+Բ��������
	double Precision_HT;                     // HTϵ��
    int Threshold_MaxModelNumPre;            // Բ���ǰԤ��ȡ���ƽ����
    int Threshold_MaxModelNum;               // ���ģ�͸���
	double Threshold_NPtsPlane;              // ƽ������������ֵ
    double Threshold_NPtsCylinder;           // Բ���������ֵϵ��
	double Threshold_DisToSurface;           // ƽ��+Բ����������ֵϵ��
	double Threshold_AngToSurface;           // ƽ��+Բ�����Ƕ���ֵ
    
    // �������ƶϲ���
	double Threshold_PRAng;                  // ƽ���ϵ�Ƕ���ֵ
	double Threshold_PRDis;                  // ƽ���ϵ������ֵϵ��
    double Threshold_PRIoU;                  // ƽ���ϵ��������ֵ
private:
    MeshDocument m_meshDoc;
    ObjSet *m_GEOObjSet;
    double m_refa;                           // ���Ȳο���λ
private:

    vcg::Point3f GetPointList(
        std::vector<int> &indexList,
        std::vector<vcg::Point3f> &pointList,
        std::vector<vcg::Point3f> &normList,
        const bool moveToCenter = false);

	// Preproc
	int DeNoiseKNN();
	int DeNoiseRegGrw();
	bool PCADimensions(
        std::vector<vcg::Point3f> &PDirections, 
        vcg::Point3f &PSize);
	double Roughness();

    
	// Detect Plane
	std::vector<ObjPatch*> DetectPlanesHT(const int expPlaneNum);
	
	// Detect Cude
    std::vector<ObjCube*> DetectCubeFromPlanes(std::vector<ObjPatch*> &patches);
	
	// Detect Cylinder
	ObjCylinder* DetectCylinderSymAxis();

    // Detect By MMF-GCO
    // [ -- !!! IMPLEMENTS ARE TOO SIMILAR !!! -- ]
    std::vector<ObjPatch*> DetectPlanesGCO(const int expPlaneNum, const int iteration = -1);
    std::vector<ObjCylinder*> DetectCylinderGCO(const int expCylinderNum, const int iteration = -1);
};

#endif // !_POINT_CLOUD_FIT_H_FILE_
