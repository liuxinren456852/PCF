#ifndef _POINT_CLOUD_FIT_H_FILE_
#define _POINT_CLOUD_FIT_H_FILE_

#include "MeshDoc.h"
#include "Satellite.h"

#ifndef _DRTrans
#define _DRTrans
#define _D2R 0.017453292
#define _R2D 57.29577951
#endif
#define _MySatePtAttri "PerVerAttri_SatePt"
typedef int SatePtType;
enum _SatePtType {
	Pt_Undefined = 0x000,
	Pt_Noise = 0x100,
	Pt_OnMBCylinder = 0x800,
	Pt_OnMBCube = 0x600,
	Pt_OnPlane = 0x0200
};

class PCFit
{
	friend void doFailure();
	friend double EIConfidence(const std::vector<int> &list);
	friend double EIConfidence(const std::vector<std::pair<double, int>> &list);

public:
	PCFit(const int nThread = 0);
	PCFit(const char *PlyFilePath, const int nThread = 0);
	~PCFit();

	void printLogo();
	void printParams();

	void setThreadNum(const int nThread);
	void initMParams(const char *iniFile = 0);

	bool loadPly(const char *PlyFilePath);
	bool savePly(const char *PlyFilePath);
	void clearD();
	void inverseD();
	int deletePts(const int mask);
	int keepPts(const int mask);
	int recolorPts(const int mask, const unsigned char R, const unsigned char G, const unsigned char B, const unsigned char A = 255);
	void autoColor();

	bool Fit_Sate(bool keepAttribute = true);
	Satellite *getSate() { return m_sate; }
private:
	MeshDocument m_meshDoc;
	Satellite *m_sate;
	double m_refa;

	// All Thresholds
	int Threshold_NPts;                      // ���������С��[Threshold_NPts]�����д���
	double RefA_Ratio;                       // ��λ���ȱ���
	int PlaneNum_Expected;                   // ����ƽ�����
	int DeNoise_MaxIteration;                // ȥ�������ֵ, Ĭ��100(<=0)
	int DeNoise_MaxNeighbors;                // ȥ��ʱKNN����ڸ���
	int DeNoise_GrowNeighbors;               // ȥ����������KNN����
	double DeNoise_DisRatioOfOutlier;        // ȥ�������������������ֵ
	double Precision_HT;                     // HTϵ��
	double Threshold_NPtsPlane;              // ƽ������������ֵ
	double Threshold_DisToPlane;             // ƽ���������ֵϵ��
	double Threshold_AngToPlane;             // ƽ����Ƕ���ֵ
	double Threshold_PRAng;                  // ƽ���ϵ�Ƕ���ֵ
	double Threshold_PRDis;                  // ƽ���ϵ������ֵϵ��
	double Threshold_NPtsCyl;                // Բ���������ֵϵ��
private:
	// Preproc
	int DeNoiseKNN();
	int DeNoiseRegGrw();
	double GetMeshSizeAlongN(const vcg::Point3f n);
	bool PCADimensionAna(vcg::Point3f &PSize, std::vector<vcg::Point3f> &PDirections, bool leftNoisePts);
	
	// Detect Plane
	std::vector<Sailboard*> DetectPlanes(const int expPN);
	
	// Refer Cude
	std::vector<Sailboard*> CuboidFaceInferring(const std::vector<Sailboard*> &planes);
	MainBodyCube* CuboidMeasure(const std::vector<Sailboard*> &CubePlanes);
	MainBodyCube* DetectCuboidFromPlanes(std::vector<Sailboard*> &planes);
	
	// Detect Cylinder
	MainBodyCylinder* DetectCylinder();    
};

#endif // !_POINT_CLOUD_FIT_H_FILE_
