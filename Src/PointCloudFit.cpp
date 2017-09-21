#include "PointCloudFit.h"
#ifdef _USE_OPENMP_
#include <omp.h>
#endif

#include <QSettings> 
#include <QTime>
#include <wrap/io_trimesh/io_mask.h>

void doFailure() {
	system("pause");
	exit(1);
}
double EIConfidence(const std::vector<int> &list)
{
	const int Num = list.size();
	const double MaxEI = -log(1.0 / Num);
	double Sumnlogn = 0.0;
	double Sumn = 0.0;
	for (int i = 0; i<Num; ++i) {
		int n = list.at(i);
		if (n>0) {
			Sumnlogn += n*log(n);
			Sumn += n;
		}
	}
	double EI = (Sumn*log(Sumn) - Sumnlogn) / (Sumn*1.0);
	return EI / MaxEI;
}
double EIConfidence(const std::vector<std::pair<double, int>> &list)
{
	const int BinNum = 10;
	const int N = list.size();
	const double min = list.at(0).first;
	const double max = list.at(N - 1).first;
	const double BinStep = (max - min) / (BinNum - 1);
	const double BeginVal = min - BinStep*0.5;
	std::vector<int> NList;
	for (int i = 0; i<BinNum; i++)
		NList.push_back(0);
	for (int i = 0; i<N; ++i) {
		double v = list.at(i).first;
		int n = (v - BeginVal) / BinStep;
		NList[n]++;
	}
	double conf = EIConfidence(NList);
	NList.clear();

	return conf;
}


PCFit::PCFit(const int nThread)
	: m_sate(0)
	, m_refa(1.0)
{
	printLogo();
	initMParams();
	setThreadNum(nThread);
}
PCFit::PCFit(const char *PlyFilePath, const int nThread)
	: m_sate(0)
	, m_refa(1.0)
{
	printLogo();
	initMParams();
	setThreadNum(nThread);
	loadPly(PlyFilePath);
}
PCFit::~PCFit()
{
	if (!m_sate) {
		delete m_sate;
		m_sate = 0;
	}
}


void PCFit::printLogo()
{
	printf(
		"\n\n"
		"    ______  ______         ______  __  _______   |  PCFit - Point Cloud Fit   \n"
		"   /  _  / / ____/  ___   / ____/ / / /__  __/   |  Ver. 1.0.0                \n"
		"  / ____/ / /___   /__/  / ___/  / /    / /      |  September 2017 @ IPC.BUAA \n"
		" /_/     /_____/        /_/     /_/    /_/       |  By WeiQM                  \n"
		"                                                 |  Email: weiqm@buaa.edu.cn  \n"
		"\n"
		"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
		"\n"
	);
}
void PCFit::printParams()
{
	printf(
		" +------------------------------------------------------+\n"
		" | PARAMETERS IN USE:                                   |\n"
		" |------------------------------------------------------|\n"
		" | [Threshold_NPts              ]:       < %000008d >   |\n"
		" | [RefA_Ratio                  ]:       < %0008.3f >   |\n"
		" | [PlaneNum_Expected           ]:       < %000008d >   |\n"
		" | [DeNoise_MaxIteration        ]:       < %000008d >   |\n"
		" | [DeNoise_MaxNeighbors        ]:       < %000008d >   |\n"			
		" | [DeNoise_DisRatioOfOutlier   ]:       < %0008.3f >   |\n"
		" | [DeNoise_GrowNeighbors       ]:       < %000008d >   |\n"
		" | [Precision_HT                ]:       < %0008.3f >   |\n"
		" | [Threshold_NPtsPlane         ]:       < %0008.3f >   |\n"
		" | [Threshold_DisToPlane        ]:       < %0008.3f >   |\n"
		" | [Threshold_AngToPlane        ]:       < %0008.3f >   |\n"		
		" | [Threshold_PRAng             ]:       < %0008.3f >   |\n"
		" | [Threshold_PRDis             ]:       < %0008.3f >   |\n"
		" | [Threshold_NPtsCyl           ]:       < %0008.3f >   |\n"
		" +------------------------------------------------------+\n",
		Threshold_NPts, RefA_Ratio, PlaneNum_Expected,
		DeNoise_MaxIteration, DeNoise_MaxNeighbors, DeNoise_DisRatioOfOutlier, DeNoise_GrowNeighbors,
		Precision_HT, Threshold_NPtsPlane,
		Threshold_DisToPlane, Threshold_AngToPlane,
		Threshold_PRAng, Threshold_PRDis,
		Threshold_NPtsCyl);
}


void PCFit::setThreadNum(const int nThread)
{
#ifdef _USE_OPENMP_
	int nThreadMax = omp_get_max_threads();
	int nThreadUsed = (nThread <= 0 || nThread > nThreadMax) ? nThreadMax : nThread;
	omp_set_num_threads(nThreadUsed);
	printf(
		" +------------------------------------------------------+\n"
		" |          USE OPENMP: %02d THREAD(s) ARE USED.          |\n"
		" +------------------------------------------------------+\n",
		nThreadUsed);
#endif // _USE_OPENMP_
}
void PCFit::initMParams(const char *iniFile)
{
	Threshold_NPts = 100;
	RefA_Ratio = 0.01;
	PlaneNum_Expected = 12;

	DeNoise_MaxIteration = 0;
	DeNoise_MaxNeighbors = 50;	
	DeNoise_DisRatioOfOutlier = 0.1;
	DeNoise_GrowNeighbors = 20;

	Precision_HT = 0.01;
	Threshold_NPtsPlane = 0.04;
	Threshold_DisToPlane = 4.0;
	Threshold_AngToPlane = 30.0;
	
	Threshold_PRAng = 15.0;
	Threshold_PRDis = 1.0/6.0;

	Threshold_NPtsCyl = 0.2;

	if (!iniFile && QFileInfo(iniFile).exists()) {
		QSettings conf(iniFile, QSettings::IniFormat);
		QStringList keys = conf.allKeys();
		if (keys.contains("Threshold_NPts"))
			Threshold_NPts = conf.value("Threshold_NPts").toInt();
		if (keys.contains("RefA_Ratio"))
			RefA_Ratio = conf.value("RefA_Ratio").toDouble();
		if (keys.contains("PlaneNum_Expected"))
			PlaneNum_Expected = conf.value("PlaneNum_Expected").toInt();

		if (keys.contains("DeNoise_MaxIteration"))
			DeNoise_MaxIteration = conf.value("DeNoise_MaxIteration").toInt();
		if (keys.contains("DeNoise_MaxNeighbors"))
			DeNoise_MaxNeighbors = conf.value("DeNoise_MaxNeighbors").toInt();
		if (keys.contains("DeNoise_DisRatioOfOutlier"))
			DeNoise_DisRatioOfOutlier = conf.value("DeNoise_DisRatioOfOutlier").toDouble();
		if (keys.contains("DeNoise_GrowNeighbors"))
			DeNoise_GrowNeighbors = conf.value("DeNoise_GrowNeighbors").toInt();

		if (keys.contains("Precision_HT"))
			Precision_HT = conf.value("Precision_HT").toDouble();
		if (keys.contains("Threshold_NPtsPlane"))
			Threshold_NPtsPlane = conf.value("Threshold_NPtsPlane").toDouble();
		if (keys.contains("Threshold_DisToPlane"))
			Threshold_DisToPlane = conf.value("Threshold_DisToPlane").toDouble();
		if (keys.contains("Threshold_AngToPlane"))
			Threshold_AngToPlane = conf.value("Threshold_AngToPlane").toDouble();

		if (keys.contains("Threshold_PRAng"))
			Threshold_PRAng = conf.value("Threshold_PRAng").toDouble();
		if (keys.contains("Threshold_PRDis"))
			Threshold_PRDis = conf.value("Threshold_PRDis").toDouble();

		if (keys.contains("Threshold_NPtsCyl"))
			Threshold_NPtsCyl = conf.value("Threshold_NPtsCyl").toDouble();
		
	}
	
}


bool PCFit::loadPly(const char * PlyFilePath)
{
	QTime time;
	time.start();
	printf("\n\n[=LoadPly=]: -->> Loading Points from File: %s <<--  \n", PlyFilePath);	
	//[----
	bool bOpen = false;
	if (PlyFilePath) {		
		bOpen = m_meshDoc.loadMesh(PlyFilePath);
		if (bOpen && m_sate != 0) {
			delete m_sate;
			m_sate = 0;
		}
	}
	//----]]
	if (bOpen)
		printf("[=LoadPly=]: Done, %d point(s) were loaded in %.4f seconds.\n", m_meshDoc.mesh->cm.vn, time.elapsed() / 1000.0);
	else {
		printf("[=LoadPly=]: [Error] Failed to open file %s for reading.\n", PlyFilePath);
		doFailure();
	}
	return bOpen;
}
bool PCFit::savePly(const char *PlyFilePath)
{
	QTime time;
	time.start();
	printf("\n\n[=SavePly=]: -->> Svae Points as Ply File: %s <<--\n", PlyFilePath);	
	//[[----
	bool bSave = m_meshDoc.saveMesh(PlyFilePath, false);
	//----]]
	if (bSave)
		printf("[=SavePly=]: Done, %d point(s) were saved in %.4f seconds.\n", m_meshDoc.mesh->cm.vn, time.elapsed() / 1000.0);
	else {
		printf("[=SavePly=]: [Error] Failed to open file %s for writing.\n", PlyFilePath);
		doFailure();
	}
	return bSave;
}

void PCFit::clearD()
{
	m_meshDoc.clearD();
}
void PCFit::inverseD()
{
	m_meshDoc.inverseD();
}

int PCFit::deletePts(const int mask)
{
	CMeshO &mesh = m_meshDoc.mesh->cm;
	int cnt = 0;
	if (vcg::tri::HasPerVertexAttribute(mesh, _MySatePtAttri))
	{
		CMeshO::PerVertexAttributeHandle<SatePtType> type_hi =
			vcg::tri::Allocator<CMeshO>::GetPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
		CMeshO::VertexIterator vi;
		for (vi = mesh.vert.begin(); vi != mesh.vert.end(); ++vi)
			if ((type_hi[vi] & mask) != 0) {
				cnt++;
				if (!vi->IsD())
					vcg::tri::Allocator<CMeshO>::DeleteVertex(mesh, *vi);
			}
	}
	return cnt;
}
int PCFit::keepPts(const int mask)
{
	CMeshO &mesh = m_meshDoc.mesh->cm;
	int cnt = 0;
	if (vcg::tri::HasPerVertexAttribute(mesh, _MySatePtAttri))
	{
		CMeshO::PerVertexAttributeHandle<SatePtType> type_hi =
			vcg::tri::Allocator<CMeshO>::GetPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
		CMeshO::VertexIterator vi;
		for (vi = mesh.vert.begin(); vi != mesh.vert.end(); ++vi) {
			if ((type_hi[vi] & mask) != 0) {
				cnt++;
				vi->ClearD();
			}
			else {
				vi->SetD();
			}
		}
	}
	mesh.vn = cnt;
	return cnt;
}

int PCFit::recolorPts(const int mask, const unsigned char R, const unsigned char G, const unsigned char B, const unsigned char A)
{
	CMeshO &mesh = m_meshDoc.mesh->cm;
	int cnt = 0;
	if (vcg::tri::HasPerVertexAttribute(mesh, _MySatePtAttri))
	{
		CMeshO::PerVertexAttributeHandle<SatePtType> type_hi =
			vcg::tri::Allocator<CMeshO>::GetPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
		CMeshO::VertexIterator vi;
		for (vi = mesh.vert.begin(); vi != mesh.vert.end(); ++vi)
		{
			if ((type_hi[vi] & mask) != 0) {
				vi->C().X() = R;
				vi->C().Y() = G;
				vi->C().Z() = B;
				vi->C().W() = A;
				++cnt;
			}
		}
	}
	return cnt;
}


bool PCFit::Fit_Sate(bool keepAttribute)
{
	if (m_meshDoc.mesh == 0 || m_meshDoc.svn() < Threshold_NPts)
		return false;

	QTime time;
	CMeshO &mesh = m_meshDoc.mesh->cm;
	m_sate = new Satellite();

	// [A+] Add Attribute
	bool bAttriAdded = false;
	{
		time.restart();
		printf("\n\n[=InitPtAttri=]: -->> Initialize Satellite Point Attribute <<--\n");		
		//[[----
		CMeshO::PerVertexAttributeHandle<SatePtType> type_hi;
		if (!vcg::tri::HasPerVertexAttribute(mesh, _MySatePtAttri))
		{// Add It
			printf("    >> Add Attri ... \n");
			type_hi = vcg::tri::Allocator<CMeshO>::GetPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
			bAttriAdded = true;
		}
		else
		{// Get It
			printf("    >> Get Attri ... \n");
			type_hi = vcg::tri::Allocator<CMeshO>::FindPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
			bAttriAdded = false;
		}
		printf("    >> Set Attri ... \n");
		for (int i = 0; i < mesh.VN(); i++)
		{// Set It
			type_hi[i] = Pt_Undefined;
		}
		//-----]]
		printf("[=InitPtAttri=]: Done, %d point(s) were setted in %.4f seconds.\n", mesh.vn, time.elapsed() / 1000.0);
	}



	// [1] A Little Preprocessing
	{
		// [1.1] Remove Outliers
		time.restart();
		printf("\n\n[=DeNoise=]: -->> Remove Outliers <<--  \n");	
		//[[----
#if 0 // DeNoise by KNN
		int nNoise = DeNoiseKNN();
#else // DeNoise by Region Grow
		int nNoise = DeNoiseRegGrw();
#endif
		//----]]
		printf("[=DeNoise=]: Done, %d outlier(s) were removed in %.4f seconds.\n", nNoise, time.elapsed()/1000.0);


		// [1.2] Get PCA Dimension
		time.restart();
		printf("\n\n[=PCASize=]: -->> PCA Dimension Ana. <<--  \n");	
		//[[----
		vcg::Point3f PSize;
		std::vector<vcg::Point3f> PDirection;
		PCADimensionAna(PSize, PDirection, true);
		m_refa = PSize.V(2)*RefA_Ratio;
		//----]]
		printf("[=PCASize=]: Done in %.4f seconds.\n", time.elapsed() / 1000.0);
	}

		
	// -- 2. Find All Planes
	std::vector<Sailboard*> planes;
	{
		printf("\n\n[=PlaneFit_HT=]: -->> Try to Fit %d Planes by Hough Translation <<--  \n", PlaneNum_Expected);
		time.restart();
		//-------------------------------
		planes = DetectPlanes(PlaneNum_Expected);
		//-------------------------------
		printf("[=PlaneFit_HT=]: Done, %d plane(s) were detected in %.4f seconds.\n", planes.size(), time.elapsed() / 1000.0);
	}

return true;

	// -- 3. Judge Cube Planes
	MainBodyCube *cubeMain = 0;
	if (planes.size() >= 2) {
		printf("\n\n[=DetectMB_Cuboid=]: -->> Try to Detect Cuboid Mainbody from %d Planes <<--  \n", planes.size());
		time.restart();
		//-------------------------------
		cubeMain = DetectCuboidFromPlanes(planes);
		m_sate->m_Mainbody = cubeMain;
		//-------------------------------
		if (cubeMain != 0) {
			printf("[=DetectMB_Cuboid=]: Done, Cuboid MB is detected in %.4f seconds, the other %d plane(s) are treated as sailbords.\n", time.elapsed() / 1000.0, planes.size());
		}
		else {
			printf("[=DetectMB_Cuboid=]: No Cuboid MB is detected, all the %d plane(s) are treated as sailbords. Elapsed %.4f seconds.\n", planes.size(), time.elapsed() / 1000.0);
		}
	}

	// -- 4. Find Cyclinde
	if (m_sate->m_Mainbody == 0) {
		if ( m_meshDoc.mesh->hasDataMask(vcg::tri::io::Mask::IOM_VERTNORMAL) ) {
			printf("\n\n[=DetectMB_Cylinder=]: -->> Try to Detect Cylinder Mainbody <<--  \n");
			time.restart();
			//-------------------------------
			MainBodyCylinder *cylinderMain = DetectCylinder();
			m_sate->m_Mainbody = cylinderMain;
			//-------------------------------
			if (cylinderMain != 0) {
				printf("[=DetectMB_Cylinder=]: Done, Cylinder MB is detected in %.4f seconds.\n", time.elapsed() / 1000.0);
			}
			else {
				printf("[=DetectMB_Cylinder=]: No Cylinder MB is detected. Elapsed %.4f seconds.\n", time.elapsed() / 1000.0);
			}
		}
		else {
			printf("\n\n[=DetectMB_Cylinder=]: [ ): ] Normals Are Neederd To Detected Cylinder Mainbody. \n");
		}
	}

	// -- 5. Set SailbordList
	{
		printf("\n\n[=SailbordCheck=]: %d plane(s) are treated as sailbords. \n", planes.size());
		m_sate->m_SailboradList = planes;
	}


	// -- *Remove Added Attribute
	if (bAttriAdded && !keepAttribute) {
		printf("\n\n[=CleanPtAttri=]: -->> Delete Point Sate Attribute <<--\n");
		vcg::tri::Allocator<CMeshO>::DeletePerVertexAttribute(mesh, _MySatePtAttri);
	}
	
	return true;
}