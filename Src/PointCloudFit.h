#ifndef _POINT_CLOUD_FIT_H_FILE_
#define _POINT_CLOUD_FIT_H_FILE_

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
#define MaxPlaneNum 256
enum _PtType {
	Pt_Undefined  = 0x0000, // 00000
	Pt_Noise      = 0x0100, // 00001
	Pt_OnPlane    = 0x0200, // 00010
    Pt_OnCube     = 0x0600, // 00110
    Pt_OnCylinder = 0x0800, // 01000
    Pt_OnCone     = 0x1800  // 11000
};


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
	int deletePts(const int mask);
	int keepPts(const int mask);
	int recolorPts(const int mask, const unsigned char r, const unsigned char g, const unsigned char b, const unsigned char a = 255);
	void autoColor();

	bool Fit_Sate(bool keepAttribute = true);
	ObjSet *getGEOObjSet() { return m_GEOObjSet; }
private:
	MeshDocument m_meshDoc;
    ObjSet *m_GEOObjSet;
	double m_refa;                           // ���Ȳο���λ

	// All Thresholds
	int Threshold_NPts;                      // ���������С��[Threshold_NPts]�����д���
	// double RefA_Ratio;                       // ��λ���ȱ���
	int PlaneNum_Expected;                   // ����ƽ�����

	int DeNoise_MaxIteration;                // ȥ�������ֵ, Ĭ��100(<=0)
	int DeNoise_KNNNeighbors;                // ȥ��ʱKNN����ڸ���(KNN)
    int DeNoise_GrowNeighbors;               // ȥ��ʱKNN����ڸ���(��������)
	double DeNoise_DisRatioOfOutlier;        // ȥ����������ֵ

	double Precision_HT;                     // HTϵ��
	double Threshold_NPtsPlane;              // ƽ������������ֵ
	double Threshold_DisToPlane;             // ƽ���������ֵϵ��
	double Threshold_AngToPlane;             // ƽ����Ƕ���ֵ
    

	double Threshold_PRAng;                  // ƽ���ϵ�Ƕ���ֵ
	double Threshold_PRDis;                  // ƽ���ϵ������ֵϵ��
    double Threshold_PRIoU;                  // ƽ���ϵ��������ֵ
	double Threshold_NPtsCyl;                // Բ���������ֵϵ��
private:
	// Preproc
	int DeNoiseKNN();
	int DeNoiseRegGrw();
	double GetMeshSizeAlongN(const vcg::Point3f n);
	bool PCADimensionAna(vcg::Point3f &PSize, std::vector<vcg::Point3f> &PDirections, bool leftNoisePts);
	double RoughnessAna(bool leftNoisePts = true);

    vcg::Point3f GetPointList(
        std::vector<int> &indexList,
        std::vector<vcg::Point3f> &pointList,
        std::vector<vcg::Point3f> &normList,
        const bool bNormalize = false);

	// Detect Plane
	std::vector<ObjPlane*> DetectPlanesHT(const int expPN);
    std::vector<ObjPlane*> DetectPlanesGCO(const int expPN, const int iteration = -1);
	
	// Detect Cude
    std::vector<ObjCube*> DetectCubeFromPlanes(std::vector<ObjPlane*> &planes);
	
	// Detect Cylinder
	ObjCylinder* DetectCylinderSymAxis();
};

#endif // !_POINT_CLOUD_FIT_H_FILE_
