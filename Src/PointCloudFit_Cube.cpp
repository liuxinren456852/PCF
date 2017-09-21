#include "PointCloudFit.h"
#include <eigenlib/Eigen/Core>
#include <eigenlib/Eigen/Eigenvalues>

// [1] Infer Cuboid Faces
bool IsOppoFaces(
	const Sailboard *P1, const Sailboard *P2,
	const double TAng)
{
	// 1.��ⷨ��ƽ��
	double ang = abs(vcg::Angle(P1->m_N, P2->m_N))*_R2D;
	if (!(90 - abs(90 - ang) < TAng))
		return false;

	// 2.����ƽ��
	double angXX = abs(vcg::Angle(P1->m_dX, P2->m_dX))*_R2D;
	double angYY = abs(vcg::Angle(P1->m_dY, P2->m_dY))*_R2D;
	double angXY = abs(vcg::Angle(P1->m_dX, P2->m_dY))*_R2D;
	double angYX = abs(vcg::Angle(P1->m_dY, P2->m_dX))*_R2D;
	if (!((90 - abs(90 - angXX) < TAng && 90 - abs(90 - angYY) < TAng) ||
		(90 - abs(90 - angXY) < TAng && 90 - abs(90 - angYX) < TAng)))
		return false;

	// 3.���λ��
	vcg::Point3f O1 = P1->m_pO + (P1->m_dX + P1->m_dY) / 2.0;
	vcg::Point3f O2 = P2->m_pO + (P2->m_dX + P2->m_dY) / 2.0;
	vcg::Point3f O2O = O1 - O2;
	double angPP1 = abs(vcg::Angle(O2O, P1->m_N))*_R2D;
	double angPP2 = abs(vcg::Angle(O2O, P2->m_N))*_R2D;
	if (90 - abs(90 - angPP1) > /*TAng*/15 || 90 - abs(90 - angPP2) > /*TAng*/15)
		return false;

	// 4.���ߴ� !!!

	// All Passed
	return true;
}
bool IsAdjacencyFaces(
	const Sailboard *P1, const Sailboard *P2,
	const double TDis, const double TAng)
{
	// 1.��ⷨ�ߴ�ֱ
	double ang = abs(vcg::Angle(P1->m_N, P2->m_N))*_R2D;
	if (!(abs(90.0 - ang)<TAng))
		return false;

	// 2.����ƽ���봹ֱ
	double angXX = abs(vcg::Angle(P1->m_dX, P2->m_dX))*_R2D;
	double angXY = abs(vcg::Angle(P1->m_dX, P2->m_dY))*_R2D;
	double angYY = abs(vcg::Angle(P1->m_dY, P2->m_dY))*_R2D;
	double angYX = abs(vcg::Angle(P1->m_dY, P2->m_dX))*_R2D;
	if (!(
		(abs(90 - angXX)<TAng && abs(90 - angXY)<TAng && (90 - abs(90 - angYY)<TAng || 90 - abs(90 - angYX)<TAng)) ||
		(abs(90 - angYX)<TAng && abs(90 - angYY)<TAng && (90 - abs(90 - angXY)<TAng || 90 - abs(90 - angXX)<TAng))
		))
		return false;

	// 3.���λ��
	vcg::Point3f O1 = P1->m_pO + (P1->m_dX + P1->m_dY) / 2.0;
	vcg::Point3f O2 = P2->m_pO + (P2->m_dX + P2->m_dY) / 2.0;
	vcg::Point3f O2O = O1 - O2;
	vcg::Point3f NN = P1->m_N^P2->m_N;
	double angPP = abs(vcg::Angle(O2O, NN))*_R2D;
	if (!(abs(90.0 - angPP) < 60.0))
		return false;

	double Dis1 = abs(O2O*P1->m_N);
	double Dis2 = abs(O2O*P2->m_N);
	double l1 = abs(90 - vcg::Angle(P1->m_dX, NN)*_R2D) < abs(90 - vcg::Angle(P1->m_dY, NN)*_R2D) ? P1->width() : P1->height();
	double l2 = abs(90 - vcg::Angle(P2->m_dX, NN)*_R2D) < abs(90 - vcg::Angle(P2->m_dY, NN)*_R2D) ? P2->width() : P2->height();
	if (!(abs(Dis1 - l2*0.5) < /*(l2*_DRT_PlaneDis)*/TDis && abs(Dis2 - l1*0.5) < TDis))
		return false;

	// 4.���ߴ� !!!

	// All Passed
	return true;
}
enum PlaneRelation {
	PlaneRelation_NoRetation,
	PlaneRelation_Adjacency,
	PlaneRelation_AtOppo
};
PlaneRelation PR(
	const Sailboard *P1, const Sailboard *P2,
	const double TDis, const double TAng)
{
	PlaneRelation  relation = PlaneRelation_NoRetation;

	if (IsOppoFaces(P1, P2, TAng))
		relation = PlaneRelation_AtOppo;
	else if (IsAdjacencyFaces(P1, P2, TDis, TAng))
		relation = PlaneRelation_Adjacency;

	return relation;
}


// [2] Estimate Orientation
bool IsParallel(const vcg::Point3f &L1, const vcg::Point3f &L2, const double AngTD)
{
	double ang = abs(vcg::Angle(L1, L2))*_R2D;

	return 90 - abs(90 - ang) < abs(AngTD);
}
bool IsPerpendicular(const vcg::Point3f &L1, const vcg::Point3f &L2, const double AngTD)
{
	double ang = abs(vcg::Angle(L1, L2))*_R2D;

	return abs(90.0 - ang) < abs(AngTD);
}
//std::pair< vcg::Point3f, double > RobustOneD(
vcg::Point3f RobustOneD(
	const std::vector< std::pair< vcg::Point3f, double > > &FaceD,
	const std::vector< std::pair< vcg::Point3f, double > > &EdgeD)
{// ������ͬ��λ�������룡����
	vcg::Point3f N;
	if (!FaceD.empty()) {
		if (FaceD.size() == 1)
			N = FaceD.at(0).first;
		else
		{
			double r1 = FaceD.at(0).second * FaceD.at(0).second;
			double r2 = FaceD.at(1).second * FaceD.at(1).second;
			N = ((FaceD.at(0).first * r2) + (FaceD.at(1).first * r1)) / ((r1 + r2) / 2.0);
		}
	}
	else {
		N.SetZero();
		double w = 0.0;
		for (int i = 0; i<EdgeD.size(); i++)
		{
			N += EdgeD.at(i).first * EdgeD.at(i).second * EdgeD.at(i).second;
			w += EdgeD.at(i).second * EdgeD.at(i).second;
		}
		N = N / w;
	}
	N.Normalize();
	return N; /// δ��һ��������
			  /*
			  std::pair< vcg::Point3f,double > retDW;
			  if (!FaceD.empty()) {
			  if (FaceD.size()==1)
			  {
			  retDW.first = FaceD.at(0).first;
			  retDW.second = 1.0;
			  }
			  else
			  {
			  double r1 = FaceD.at(0).second * FaceD.at(0).second;
			  double r2 = FaceD.at(1).second * FaceD.at(1).second;
			  retDW.first = ((FaceD.at(0).first * r2) + (FaceD.at(1).first * r1))/((r1+r2)/2.0);
			  retDW.second = 1.0 + FaceD.at(0).first*FaceD.at(1).first;
			  }
			  } else {
			  vcg::Point3f d;
			  double w;
			  d.SetZero();
			  w = 0.0;
			  for (int i=0; i<EdgeD.size(); i++)
			  {
			  d += EdgeD.at(i).first * EdgeD.at(i).second * EdgeD.at(i).second;
			  if (EdgeD.at(i).second>w)
			  w = EdgeD.at(i).second;
			  }
			  retDW.first = d;
			  retDW.second = w;
			  }
			  retDW.first.Normalize();
			  return retDW; /// δ��һ��������
			  */
}
void RobustDirections(
	const std::vector< std::pair< vcg::Point3f, double > > &FaceNX,
	const std::vector< std::pair< vcg::Point3f, double > > &FaceNY,
	const std::vector< std::pair< vcg::Point3f, double > > &FaceNZ,
	const std::vector< std::pair< vcg::Point3f, double > > &EdgeNX,
	const std::vector< std::pair< vcg::Point3f, double > > &EdgeNY,
	const std::vector< std::pair< vcg::Point3f, double > > &EdgeNZ,
	vcg::Point3f &NX, vcg::Point3f &NY, vcg::Point3f &NZ)
{
	vcg::Point3f nx = RobustOneD(FaceNX, EdgeNX);
	vcg::Point3f ny = RobustOneD(FaceNY, EdgeNY);
	vcg::Point3f nz = RobustOneD(FaceNZ, EdgeNZ);

	Eigen::Matrix3f A;
	A(0, 0) = nx.X(), A(0, 1) = nx.Y(), A(0, 2) = nx.Z();
	A(1, 0) = ny.X(), A(1, 1) = ny.Y(), A(1, 2) = ny.Z();
	A(2, 0) = nz.X(), A(2, 1) = nz.Y(), A(2, 2) = nz.Z();
	Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeThinU | Eigen::ComputeThinV);
	Eigen::Matrix3f V = svd.matrixV(), U = svd.matrixU();
	Eigen::Matrix3f S = U * Eigen::Matrix3f::Identity() * V.transpose(); // S = U^-1 * A * VT * -1  
	nx = vcg::Point3f(S(0, 0), S(0, 1), S(0, 2));
	ny = vcg::Point3f(S(1, 0), S(1, 1), S(1, 2));
	nz = vcg::Point3f(S(2, 0), S(2, 1), S(2, 2));

	NX = nx;
	NY = ny;
	NZ = nz;

	/*
	std::pair< vcg::Point3f,double > NXW = RobustOneD(FaceNX,EdgeNX);
	std::pair< vcg::Point3f,double > NYW = RobustOneD(FaceNY,EdgeNY);
	std::pair< vcg::Point3f,double > NZW = RobustOneD(FaceNZ,EdgeNZ);

	NX = NXW.first;
	NY = NYW.first;
	NZ = NZW.first;
	if (NXW.second > NYW.second && NXW.second > NZW.second) {
	if (NYW.second > NZW.second)
	{// 1. X>Y>Z
	NZ = NX^NY;
	NY = NZ^NX;
	}
	else
	{// 2. X>Z>Y
	NY = NZ^NX;
	NZ = NX^NY;
	}
	} else if (NYW.second > NZW.second) {
	if (NZW.second > NXW.second)
	{// 3. Y>Z>X
	NX = NY^NZ;
	NZ = NX^NY;
	}
	else
	{// 4. Y>X>Z
	NZ = NX^NY;
	NX = NY^NZ;
	}
	} else {
	if (NYW.second > NXW.second)
	{// 5. Z>Y>X
	NX = NY^NZ;
	NY = NZ^NX;
	}
	else
	{// 6. Z>X>Y
	NY = NZ^NX;
	NX = NY^NZ;
	}
	}
	NX.Normalize();
	NY.Normalize();
	NZ.Normalize();
	*/
}
void RobustOrientation(
	const std::vector<Sailboard*> &CubePlanes,
	vcg::Point3f &NX, vcg::Point3f &NY, vcg::Point3f &NZ,
	const double TAng)
{
	Sailboard *PP = CubePlanes.at(0);
	vcg::Point3f NXRef = PP->m_dX; NXRef.Normalize();
	vcg::Point3f NYRef = PP->m_dY; NYRef.Normalize();
	vcg::Point3f NZRef = PP->m_N; NZRef.Normalize();
	double ZVar = PP->m_varN;
	double XYWeight = std::min(PP->m_sizeConfidence.X(), PP->m_sizeConfidence.Y());
	std::vector< std::pair< vcg::Point3f, double > > FaceNX, FaceNY, FaceNZ; // ���淨���ṩ��ָ����Ϣ
	std::vector< std::pair< vcg::Point3f, double > > EdgeNX, EdgeNY, EdgeNZ; // �ɱ߷����ṩ��ָ����Ϣ
	FaceNZ.push_back(std::pair<vcg::Point3f, double>(NZRef, ZVar));
	EdgeNX.push_back(std::pair<vcg::Point3f, double>(NXRef, XYWeight));
	EdgeNY.push_back(std::pair<vcg::Point3f, double>(NYRef, XYWeight));

	vcg::Point3f *pX, *pY, *pZ;
	std::vector< std::pair< vcg::Point3f, double > > *pXV, *pYV, *pZV;
	for (int i = 1; i<CubePlanes.size(); ++i)
	{
		PP = CubePlanes.at(i);
		vcg::Point3f nx = PP->m_dX;
		vcg::Point3f ny = PP->m_dY;
		vcg::Point3f nz = PP->m_N;
		double zvar = PP->m_varN;
		double xyweight = std::min(PP->m_sizeConfidence.X(), PP->m_sizeConfidence.Y());
		if (IsParallel(nz, NZRef, TAng))
		{ // ƽ��
			pZ = &NZRef;
			pZV = &FaceNZ;
			if (IsParallel(nx, NXRef, TAng)) {
				pX = &NXRef;
				pXV = &EdgeNX;
				pY = &NYRef;
				pYV = &EdgeNY;
			}
			else {
				pX = &NYRef;
				pXV = &EdgeNY;
				pY = &NXRef;
				pYV = &EdgeNX;
			}
		}
		else
		{ // ��ֱ
		  // ͬ�������������ü���
			if (IsParallel(nz, NXRef, TAng)) {
				pZ = &NXRef;
				pZV = &FaceNX;

				pX = &NZRef;
				pXV = &EdgeNZ;
				pY = &NYRef;
				pYV = &EdgeNY;
			}
			else {
				pZ = &NYRef;
				pZV = &FaceNY;

				pX = &NXRef;
				pXV = &EdgeNX;
				pY = &NZRef;
				pYV = &EdgeNZ;
			}

			// ������෴->�滻λ��
			if (!(IsParallel(nx, NXRef, TAng) ||
				IsPerpendicular(nx, NXRef, TAng))) {
				vcg::Point3f *pTemp = pX;
				std::vector< std::pair< vcg::Point3f, double > > *pVTemp = pXV;
				pX = pY;
				pXV = pYV;
				pY = pTemp;
				pYV = pVTemp;
			}
		}

		if (*pX*nx<0) nx = -nx;
		if (*pY*ny<0) ny = -ny;
		if (*pZ*nz<0) nz = -nz;
		nx.Normalize();
		ny.Normalize();
		nz.Normalize();
		pZV->push_back(std::pair<vcg::Point3f, double>(nz, zvar));
		pXV->push_back(std::pair<vcg::Point3f, double>(nx, xyweight));
		pYV->push_back(std::pair<vcg::Point3f, double>(ny, xyweight));
	}
	assert((FaceNX.size() + EdgeNX.size()) == CubePlanes.size());
	assert((FaceNY.size() + EdgeNY.size()) == CubePlanes.size());
	assert((FaceNZ.size() + EdgeNZ.size()) == CubePlanes.size());
	RobustDirections(FaceNX, FaceNY, FaceNZ, EdgeNX, EdgeNY, EdgeNZ, NX, NY, NZ);
}


// [3] Estimate Dimenstion
std::pair< double, double > FaceInter(
	const vcg::Point3f &NRef,
	const vcg::Point3f &O,
	const vcg::Point3f &NX,
	const vcg::Point3f &NY)
{
	double sum = 0.0;
	double sum2 = 0.0;
	double loc = 0.0;
	loc = NRef*O;			sum += loc;	sum2 += loc*loc;
	loc = NRef*(O + NX);		sum += loc;	sum2 += loc*loc;
	loc = NRef*(O + NY);		sum += loc;	sum2 += loc*loc;
	loc = NRef*(O + NX + NY);	sum += loc;	sum2 += loc*loc;
	double locMiu = sum / 4.0;
	double locVar = sqrt(sum2 / 4.0 - locMiu*locMiu);
	return std::pair< double, double >(locMiu, locVar);
}
std::pair< double, double > LineProj(
	const vcg::Point3f &NRef,
	const vcg::Point3f &O,
	const vcg::Point3f &NMain,
	const vcg::Point3f &NSecned)
{
	double loc11 = NRef*O;
	double loc12 = NRef*(O + NMain);
	double loc21 = NRef*(O + NSecned);
	double loc22 = NRef*(O + NMain + NSecned);

	double loc1 = (loc11 + loc21) / 2.0;
	double loc2 = (loc12 + loc22) / 2.0;
	if (loc1>loc2)
		return std::pair< double, double >(loc1, loc2);
	else
		return std::pair< double, double >(loc2, loc1);
}
double RobustLengthOneD(
	const std::vector< std::pair< double, double > > &FaceInter,
	std::vector< std::pair< std::pair< double, double >, std::pair< double, double > > > &EdgeInter,
	double &minProj, double &maxProj)
{
	double w = 0.0;

	if (FaceInter.size() >= 2) {
		if (FaceInter.at(0).first<FaceInter.at(1).first) {
			minProj = FaceInter.at(0).first;
			maxProj = FaceInter.at(1).first;
		}
		else {
			minProj = FaceInter.at(1).first;
			maxProj = FaceInter.at(0).first;
		}
		double ll = maxProj - minProj;
		w = (1.0 - FaceInter.at(0).second / ll) * (1.0 - FaceInter.at(1).second / ll);
	}
	else {
		int N = EdgeInter.size();
		std::vector<std::pair< double, double >> upList, downList;
		for (int i = 0; i<N; i++) {
			upList.push_back(EdgeInter.at(i).first);
			downList.push_back(EdgeInter.at(i).second);
		}

		double up, upw, down, downw;
		//up = upw = down = downw = 0.0;
		//for (int i = 0; i < N; ++i) {
		//	up += upList.at(i).first;
		//	upw += upList.at(i).second;
		//	down += downList.at(i).first;
		//	downw += downList.at(i).second;
		//}
		//up = up / N;
		//upw = upw / N;
		//down = down / N;
		//downw = downw / N;

		std::sort(upList.begin(), upList.end());
		std::sort(downList.begin(), downList.end());
		if (N % 2 == 0) {
			int index1 = N / 2 - 1;
			int index2 = N / 2;
			up = (upList.at(index1).first + upList.at(index2).first) / 2.0;
			upw = (upList.at(index1).second + upList.at(index2).second) / 2.0;
			down = (downList.at(index1).first + downList.at(index2).first) / 2.0;
			downw = (downList.at(index1).second + downList.at(index2).second) / 2.0;
		}
		else {
			int index = N / 2;
			up = upList.at(index).first;
			upw = upList.at(index).second;
			down = downList.at(index).first;
			downw = downList.at(index).second;
		}

		if (FaceInter.size() != 0) {
			double fi = FaceInter.at(0).first;
			double var = FaceInter.at(0).second;
			if (abs(fi - up)<abs(fi - down)) {
				up = fi;
				upw = 1.0 - var / (up - down);
			}
			else {
				down = fi;
				downw = 1.0 - var / (up - down);
			}
		}
		minProj = down;
		maxProj = up;
		w = upw*downw;
	}
	return w;
}
void RobustDimention(
	const std::vector<Sailboard*> &CubePlanes,
	vcg::Point3f &NX, vcg::Point3f &NY, vcg::Point3f &NZ,
	vcg::Point3f &OP, vcg::Point3f &SIZE, vcg::Point3f &WEIGHTS,
	const double TAng)
{
	// <λ��,Ȩֵ>
	std::vector< std::pair< double, double > > FaceInterX, FaceInterY, FaceInterZ;
	std::vector< std::pair< std::pair< double, double >, std::pair< double, double > > > EdgeInterX, EdgeInterY, EdgeInterZ;

	vcg::Point3f *pNX, *pNY, *pNZ;
	std::vector< std::pair< double, double > > *pFZ;
	std::vector< std::pair< std::pair< double, double >, std::pair< double, double > > > *pEX, *pEY;
	for (int i = 0; i<CubePlanes.size(); ++i)
	{
		Sailboard *PP = CubePlanes.at(i);
		vcg::Point3f op = PP->m_pO;
		vcg::Point3f nx = PP->m_dX;
		vcg::Point3f ny = PP->m_dY;
		vcg::Point3f nz = PP->m_N;
		double xyweight = std::min(PP->m_sizeConfidence.X(), PP->m_sizeConfidence.Y());
		if (IsParallel(nz, NX, TAng))
		{ // YZƽ��ƽ��
			pNZ = &NX;
			pFZ = &FaceInterX;
			if (IsParallel(nx, NY, TAng))
			{ // x//Y y//Z
				pNX = &NY;
				pEX = &EdgeInterY;
				pNY = &NZ;
				pEY = &EdgeInterZ;
			}
			else
			{// x//Z y//Y
				pNX = &NZ;
				pEX = &EdgeInterZ;
				pNY = &NY;
				pEY = &EdgeInterY;
			}
		}
		else if (IsParallel(nz, NY, TAng))
		{ // XZƽ��ƽ��
			pNZ = &NY;
			pFZ = &FaceInterY;
			if (IsParallel(nx, NX, TAng))
			{ // x//X y//Z
				pNX = &NX;
				pEX = &EdgeInterX;
				pNY = &NZ;
				pEY = &EdgeInterZ;
			}
			else
			{// x//Z y//X
				pNX = &NZ;
				pEX = &EdgeInterZ;
				pNY = &NX;
				pEY = &EdgeInterX;
			}
		}
		else
		{ // XYƽ��ƽ��
			pNZ = &NZ;
			pFZ = &FaceInterZ;
			if (IsParallel(nx, NX, TAng))
			{ // x//X y//Y
				pNX = &NX;
				pEX = &EdgeInterX;
				pNY = &NY;
				pEY = &EdgeInterY;
			}
			else
			{// x//Y y//X
				pNX = &NY;
				pEX = &EdgeInterY;
				pNY = &NX;
				pEY = &EdgeInterX;
			}
		}
		std::pair< double, double > IF = FaceInter(*pNZ, op, nx, ny);
		pFZ->push_back(IF);

		std::pair< double, double > IX = LineProj(*pNX, op, nx, ny);
		pEX->push_back(
			std::pair< std::pair< double, double >, std::pair< double, double > >(
				std::pair< double, double >(IX.first, xyweight),
				std::pair< double, double >(IX.second, xyweight)
				)
		);
		std::pair< double, double > IY = LineProj(*pNY, op, ny, nx);
		pEY->push_back(
			std::pair< std::pair< double, double >, std::pair< double, double > >(
				std::pair< double, double >(IY.first, xyweight),
				std::pair< double, double >(IY.second, xyweight)
				)
		);

	}
	assert((FaceInterX.size() + EdgeInterX.size()) == CubePlanes.size());
	assert((FaceInterY.size() + EdgeInterY.size()) == CubePlanes.size());
	assert((FaceInterZ.size() + EdgeInterZ.size()) == CubePlanes.size());

	double minx, miny, minz, maxx, maxy, maxz;
	double wx = RobustLengthOneD(FaceInterX, EdgeInterX, minx, maxx);
	double wy = RobustLengthOneD(FaceInterY, EdgeInterY, miny, maxy);
	double wz = RobustLengthOneD(FaceInterZ, EdgeInterZ, minz, maxz);
	SIZE = vcg::Point3f(maxx - minx, maxy - miny, maxz - minz);
	WEIGHTS = vcg::Point3f(wx, wy, wz);
	OP = NX*minx + NY*miny + NZ*minz;

}


// [4] Incorporation
bool BelongToCube(
	const Sailboard &plane,
	const MainBodyCube &Cube,
	const double TAng)
{
	double lx = Cube.DimX();
	double ly = Cube.DimY();
	double lz = Cube.DimZ();
	vcg::Point3f centerCube = Cube.m_pO + (Cube.m_dX + Cube.m_dY + Cube.m_dZ) / 2.0;

	vcg::Point3f np = plane.m_N;
	vcg::Point3f centerPlane = plane.m_pO + (plane.m_dX + plane.m_dY) / 2.0;

	double lt, la, lb;
	vcg::Point3f nt, na, nb;
	// 1.ƽ��
	if (IsParallel(np, Cube.m_dX, TAng*2.0)) {
		lt = lx; nt = Cube.m_dX;
		la = ly; na = Cube.m_dY;
		lb = lz; nb = Cube.m_dZ;
	}
	else if (IsParallel(np, Cube.m_dY, TAng*2.0)) {
		lt = ly; nt = Cube.m_dY;
		la = lx; na = Cube.m_dX;
		lb = lz; nb = Cube.m_dZ;
	}
	else if (IsParallel(np, Cube.m_dZ, TAng*2.0)) {
		lt = lz; nt = Cube.m_dZ;
		la = ly; na = Cube.m_dY;
		lb = lx; nb = Cube.m_dX;
	}
	else
		return false;

	// 2.����
	nt.Normalize();
	na.Normalize();
	nb.Normalize();
	double disN = abs(nt*(centerCube - centerPlane));
	double thresholdN = lt / 2.0;
	double disA = abs((centerCube - centerPlane)*na);
	double disB = abs((centerCube - centerPlane)*nb);
	double thresholdT = sqrt(lx*lx + ly*ly + lz*lz - lt*lt) / 2.0;
	if (abs(disN - thresholdN) < thresholdN*0.2 &&
		disA < la / 2.0 && disB < lb / 2.0)
		return true;
	else
		return false;
}



std::vector<Sailboard*> PCFit::CuboidFaceInferring(const std::vector<Sailboard*> &planes)
{
	std::vector<Sailboard*> tempList;
	std::vector<Sailboard*> finalOut;
	const double TDis = Threshold_PRDis * m_refa;
	const double TAng = Threshold_PRAng;
	for (int i = 0; i < planes.size(); ++i) {
		tempList.clear();
		tempList.push_back(planes.at(i));
		for (int j = 0; j < planes.size(); j++) {
			if (j == i)
				continue;
			bool isCuboidFace = true;
			for (int k = 0; k < tempList.size(); k++) {
				if (PR(tempList.at(k), planes.at(j), TDis, TAng) == PlaneRelation_NoRetation)
				{
					isCuboidFace = false;
					break;
				}
			}
			if (isCuboidFace)
				tempList.push_back(planes.at(j));
		}
		if (tempList.size() > finalOut.size())
			finalOut = tempList;
	}
	tempList.clear();
	return finalOut;
}
MainBodyCube* PCFit::CuboidMeasure(const std::vector<Sailboard*> &CubePlanes)
{
	MainBodyCube *cube = new MainBodyCube();
	if (CubePlanes.size()>6) {
#ifdef D_D_D_DBUG
		MsgOut::_msgout("Funny Thing: A Cube With More Than 6 Faces. Oops!");
#endif
	}
	else {
#ifdef D_D_D_DBUG
		MsgOut::_msgout(QString("Measure Cube With %1 Faces.").arg(CubePlanes.size()));
#endif
	}
	// 1.���Ʒ���
	vcg::Point3f NX, NY, NZ;
	RobustOrientation(CubePlanes, NX, NY, NZ, Threshold_PRAng);
	// 2.����λ�úͳߴ�
	vcg::Point3f OP, size, sizeweights;
	RobustDimention(CubePlanes, NX, NY, NZ, OP, size, sizeweights, Threshold_PRAng);

	cube->m_dX = NX*size.X();
	cube->m_dY = NY*size.Y();
	cube->m_dZ = NZ*size.Z();
	cube->m_pO = OP;
	cube->m_sizeConfidence = sizeweights;
	return cube;
}
MainBodyCube* PCFit::DetectCuboidFromPlanes(std::vector<Sailboard*> &planes)
{
	CMeshO &mesh = m_meshDoc.mesh->cm;

	if (planes.size()<2) // �ϻ�
		return 0;

	// A.ɸѡ������
	std::vector<Sailboard*> CuboidFaces = CuboidFaceInferring(planes);
	if (CuboidFaces.size() < 2)
		return 0;

#ifdef D_D_D_DBUG
	MsgOut::_msgout(QString("Candicate Cuboid With %1 Faces.").arg(CuboidFaces.size()));
#endif

	// B.����Cuboidָ��+λ��+�ߴ�
	MainBodyCube *Cube = CuboidMeasure(CuboidFaces);

	// C.�ٴ��ж�
	int NAdded = 0;
	for (int i = 0; i < planes.size(); i++) {
		Sailboard *sb = planes.at(i);
		bool bCuboidFace = false;
		for (int k = 0; k < CuboidFaces.size(); k++) {
			if (CuboidFaces.at(k) == sb) {
				bCuboidFace = true;
				break;
			}
		}
		if (!bCuboidFace && BelongToCube(*sb, *Cube, Threshold_PRAng)) {
			CuboidFaces.push_back(sb);
			NAdded++;
		}
	}

#ifdef D_D_D_DBUG
	MsgOut::_msgout(QString("Incorporate %1 Other Faces.").arg(NAdded));
#endif

	// D.�������
	for (int i = 0; i < CuboidFaces.size(); i++)
	{//��С����
		Sailboard *sb = CuboidFaces.at(i);
		for (int k = 0; k < planes.size(); k++) {
			if (planes.at(k) == sb) {
				planes.erase(planes.begin() + k);
				break;
			}
		}
	}
	// E.���ñ�ǩ
	CMeshO::PerVertexAttributeHandle<SatePtType> type_hi = vcg::tri::Allocator<CMeshO>::FindPerVertexAttribute<SatePtType>(mesh, _MySatePtAttri);
	CMeshO::VertexIterator vi;
	for (vi = mesh.vert.begin(); vi != mesh.vert.end(); vi++) {
		if (!(*vi).IsD()) {
			int code = type_hi[vi];
			for (int k = 0; k < CuboidFaces.size(); k++) {
				if (code == CuboidFaces.at(k)->m_PlaneIndex) {
					type_hi[vi] = Pt_OnMBCube;
					break;
				}
			}
		}
	}

	return Cube;
}