#pragma once

#include <opencv2/imgproc/imgproc.hpp>  
#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O
#include <fstream>
#include <iostream> // for standard I/O
#include <queue>

#include <QString>
#include <QPoint>
#include <qhash.h>

#define I_MAX 255
//#define PREDEFINED_THRESHOLD 150 //156
#define X_SIZE 640
#define Y_SIZE 480
#define Z_SIZE 8000
#define MIN_Z 10
#define MAX_Z Z_SIZE
#define NORMALIZATION_INTERVAL 2000 //(MAX_Z - MIN_Z) // ==> no normalization

//to calculate volume object differences:
#define X_TOLERANCE 0
#define Y_TOLERANCE 0
#define Z_TOLERANCE 0



using namespace std;

inline uint qHash(const QPoint& r)
{
    return qHash(QString("%1,%2").arg(r.x()).arg(r.y()));
}

class VmtFunctions
{
private:
	static const int dims = 3;
	int *matrixSize;
    unsigned int permittedMinZ;
    unsigned int permittedMaxZ;

    unsigned int toleranceX;
    unsigned int toleranceY;
    unsigned int toleranceZ;

	enum DimIndex
	{
        X = 0,
        Y = 1,
		Z = 2
	};

	inline float raw_depth_to_meters(int raw_depth)
	{
        if (raw_depth <= 2047)
			return 1.0 / (raw_depth * -0.0030711016 + 3.3309495161);
		return 0;
	}

    struct VmtInfo
    {
        int numberOfPoints;
        int sizeInX;
        int sizeInY;
        int sizeInZ;
        int maxX;
        int maxY;
        int maxZ;
        int minX;
        int minY;
        int minZ;
    };

public:
	//constructor for 3D VMTs
    VmtFunctions(int xSize = X_SIZE, int ySize = Y_SIZE, unsigned int tolX = X_TOLERANCE, unsigned int tolY = Y_TOLERANCE, unsigned int tolZ = Z_TOLERANCE);
	~VmtFunctions(void);

    //generate a VMT from a video folder and a sliding window (generated by Bingbing's algo, 40frames long)
    cv::SparseMat ConstructSparseVMT(QString videoFolderPath, QString trackFilePath, int downsamplingRate = 1);

    static void Print3x3Matrix(const cv::Matx33d& mat);
    static void Print3DSparseMatrix(const cv::SparseMat& sparse_mat);
    static void Save3DSparseMatrix(const cv::SparseMat& sparse_mat, QString filePath);
    static void Save3DMatrix(const cv::Mat& sparse_mat, QString filePath);
    static void Save3DSparseMatrixAs2DCsv(const cv::SparseMat& sparse_mat, QString filePath);



protected:

    //to be used with cropped images (with track files)
    cv::SparseMat GenerateSparseVolumeObject(cv::Mat image, int downsamplingRate = 2);

    cv::SparseMat SubtractSparseMat(const cv::SparseMat& operand1, const cv::SparseMat& operand2);

    cv::SparseMat CleanUpVolumeObjectDifference(const cv::SparseMat& volObjDiff) const;

    double AttenuationConstantForAnAction(const QList<cv::SparseMat> &volumeObjectsDifferences);

    //magnitude of motion is calculated over a volume object difference (delta) at a given time t
    int MagnitudeOfMotion(const cv::SparseMat& sparseMat);

    //A VMT is constructed over a sequence of volume object differences
    cv::SparseMat ConstructVMT(const QList<cv::SparseMat>& volumeObjectDifferences);

    cv::SparseMat CalculateD_Old(cv::SparseMat lastVolumeObject, cv::SparseMat firstVolumeObject);
    cv::SparseMat CalculateD_New(cv::SparseMat lastVolumeObject, cv::SparseMat firstVolumeObject);
    cv::Vec3i CalculateMomentVector(cv::SparseMat volumeObject);

    double CalculateAlpha(cv::Vec3i motionVector);
    double CalculateBeta(cv::Vec3i motionVector);
    double CalculateTheta(cv::Vec3i motionVector);

    cv::Matx33d CalculateRotationX_alpha(double alpha);
    cv::Matx33d CalculateRotationY_beta(double beta);
    cv::Matx33d CalculateRotationZ_theta(double theta);

    cv::SparseMat RotateVMT(const cv::SparseMat& vmt, const cv::Matx33d& rotationMatrix);
    cv::Mat ProjectVMTOntoXY(const cv::SparseMat& vmt);

    //Get basic info of a 3d sparse mat
    VmtInfo GetVmtInfo(const cv::SparseMat &vmt) const;
    //shrink & trim the 3d sparse mat to get rid of unnecessarily large size
    cv::SparseMat TrimSparseMat(const cv::SparseMat &vmt);

    cv::SparseMat SpatiallyNormalizeSparseMat(cv::SparseMat vmt);

    //not used
    //    cv::Mat GenerateVolumeObject(cv::Mat image, int downsamplingRate = 2);
    //    vector<cv::SparseMat> CalculateVolumeObjectDifferencesSparse(const vector<cv::SparseMat>& volumeObjects);
    //    vector<cv::SparseMat> CalculateVolumeObjectDifferencesSparse(const vector<cv::SparseMat>& volumeObjects, int depthTolerance);
    //    cv::SparseMat GenerateSparseVolumeObject(QString imagePath, int downsamplingRate = 2);
    //    vector<cv::SparseMat> ConstructVMTs(const vector<cv::SparseMat>& volumeObjectDifferences);

};



