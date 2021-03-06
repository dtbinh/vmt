#include "VmtFunctions.h"
#include "HelperFunctions.h"

#include <stdio.h>

#include <qfile.h>
#include <qtextstream.h>


VmtFunctions::VmtFunctions(int xSize, int ySize)
{
	this->permittedMinZ = MIN_Z;
	this->permittedMaxZ = MAX_Z;

	this->matrixSize = new int[this->dims];
	this->matrixSize[Y] = ySize;
	this->matrixSize[X] = xSize;
	this->matrixSize[Z] = this->permittedMaxZ - this->permittedMinZ;	
}


VmtFunctions::~VmtFunctions(void)
{
	delete[] matrixSize;
}

cv::SparseMat VmtFunctions::GenerateSparseVolumeObject(string imagePath, string windowName, int downsamplingRate)
{
	cv::SparseMat emptySparseMat;
	cv::Mat image, temp;	

	//Read the file 
	image = cv::imread(imagePath, CV_LOAD_IMAGE_UNCHANGED);   //ATTENTION: LIRIS DATA HAS 16BIT DEPTH IMAGES	
	//Check for invalid input
	if(! image.data )                              
	{
		std::cout <<  "Could not open or find the image: " << imagePath << endl ;
		return emptySparseMat;
	}

	image.convertTo(temp, CV_16UC1);
	image.release();
	temp.assignTo(image);
	temp.release();	

	//THRESHOLD TO GET SILHOUETTE
	//cv::Mat image8bit;
	//image.convertTo(image8bit, CV_8UC1, 1.0/256.0);
	//cv::adaptiveThreshold(image8bit, binImage, I_MAX, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, 3, 0);	

	//Generate Volume Object:	
	cv::SparseMat sparse_mat(this->dims, this->matrixSize, CV_8UC1);

	unsigned short pixelValue;
	unsigned short mostSig13Digits;
	unsigned short depth;
	unsigned int depthInMillimeters;

	//NOTE: because of memory restrictions, x and y are incremented by 2 (~downsampling by 1/4)
	for (int y = 0; y < image.rows-1; y+=downsamplingRate)
	{ 
		const unsigned short* ithRow = image.ptr<unsigned short>(y); //take a whole row

		for (int x = 0; x < image.cols-1; x+=downsamplingRate)
		{ 
			pixelValue = ithRow[x]; 
			mostSig13Digits = pixelValue & 65504; // (65504 = 1111 1111 1110 0000)
			depth = mostSig13Digits >> 5;

            depthInMillimeters = (unsigned int)(raw_depth_to_meters((int)depth)*1000);

			if (depthInMillimeters >= this->permittedMinZ && depthInMillimeters <= this->permittedMaxZ) //to discard depth values not between the permitted range
			{
				depthInMillimeters -= this->permittedMinZ; //translate all points to fit between min-max permitted range
				sparse_mat.ref<uchar>(y, x, depthInMillimeters) = I_MAX; 
			}
		}
	}	

	image.release();

	return sparse_mat;
}

cv::SparseMat VmtFunctions::GenerateSparseVolumeObject(string imagePath, string textPath, string windowName)
{
	cv::SparseMat emptySparseMat;
//	cv::Mat image;
//	cv::Mat binImage;
//	cv::Mat binImage2;
//	image = cv::imread(imagePath , CV_LOAD_IMAGE_GRAYSCALE);   // Read the file

//	if(! image.data )                              // Check for invalid input
//	{
//		std::cout <<  "Could not open or find the image: " << imagePath << endl ;
//		return emptySparseMat;
//	}

//	cv::threshold(image, binImage, PREDEFINED_THRESHOLD, I_MAX, cv::THRESH_BINARY);
//	binImage.convertTo(binImage2, CV_8UC1);
//	cv::imshow(windowName, binImage2);
//	cv::waitKey(10);

//	ifstream fileStream(textPath);
//	string line = "";

//    std::vector<std::vector<short> > depthData;

//	if (fileStream.is_open())
//	{
//		short buffer = 0;
//		int counter = 0;
//		std::vector<short> depthRow;

//		while ( getline (fileStream,line) )
//		{
//			if (counter % this->matrixSize[X] == 0) //beginning of a new row
//			{
//				if (depthRow.size() == this->matrixSize[X])
//				{
//					depthData.push_back(depthRow);
//					depthRow.clear();
//				}
//			}
//			buffer = atoi(line.c_str());
//			depthRow.push_back((short)buffer);

//			++counter;
//		}
//	}
//	else
//	{
//		cout <<  "Could not open or find the text data" << endl ;
//		return emptySparseMat;
//	}

//	//Generate Volume Object:
//	cv::SparseMat sparse_mat(this->dims, matrixSize, CV_8UC1);

//	int idx[this->dims];

//	for (int i = 0; i < binImage2.rows-1; i++) //y
//	{
//		for (int j = 0; j < binImage2.cols-1; j++) //x
//		{
//			if (binImage2.at<uchar>(i, j) > 0) //for non zero values of binary image
//			{
//				short depth = depthData[i][j];
//				if (depth > 0)
//				{
//					idx[Y] = i; //y
//					idx[X] = j; //x

//					idx[Z] =  depth; //z
//					sparse_mat.ref<uchar>(idx) = I_MAX;
//				}
//			}
//		}
//	}

    return emptySparseMat;
}

vector<cv::SparseMat> VmtFunctions::CalculateVolumeObjectDifferencesSparse(const vector<cv::SparseMat>& volumeObjects)
{
	cv::SparseMat operand1, operand2;
	vector<cv::SparseMat> volumeObjectDifferences;
	short counter = 2;

    for (std::vector<cv::SparseMat>::const_iterator it = volumeObjects.begin() ; it != volumeObjects.end(); ++it)
	{
        if (it == volumeObjects.begin()) //skip the first one
		{	
			it->assignTo(operand2);
			continue;
		}

		operand1.release();
		operand2.copyTo(operand1);
		operand2.release();
		it->assignTo(operand2);		

		// now operand2 is current volume object, operand1 is previous volume object
		//diff(x, y, z) = |operand2(x,y,z) - operand1(x,y,z)|
		//				= 1 if |1 - 0| or |0 - 1|
		//				= 0 if |0 - 0| or |1 - 1|

		//init difference:
		cv::SparseMat difference = cv::SparseMat(this->dims, this->matrixSize, operand2.type());

		//go through nonzero values of operand2, and set diff to 1 if operand1(x,y,z)==0		
		for (cv::SparseMatConstIterator opIt=operand2.begin(); opIt != operand2.end(); ++opIt)
		{
			const cv::SparseMat::Node* n = opIt.node();
			uchar val2 = opIt.value<uchar>();
			uchar val1 = operand1.value<uchar>(n->idx);
			if (val1 <= 0)
				difference.ref<uchar>(n->idx) = val2;
		}
		//go through nonzero values of operand1, and set diff to 1 if operand2(x,y,z)==0		
		for (cv::SparseMatConstIterator opIt=operand1.begin(); opIt != operand1.end(); ++opIt)
		{
			const cv::SparseMat::Node* n = opIt.node();
			uchar val1 = opIt.value<uchar>();
			uchar val2 = operand2.value<uchar>(n->idx);			
			if (val2 <= 0)
				difference.ref<uchar>(n->idx) = val1;
		}

		//all of the remaining points of difference should be equal to 0.

		cout << "Difference calculated [" << counter << ", " << counter - 1 << "], nonzero: " << (int)difference.nzcount() << endl;
		volumeObjectDifferences.push_back(difference);		
		counter++;
		difference.release();
	}
	operand1.release();
	operand2.release();

	return volumeObjectDifferences;
}

cv::SparseMat VmtFunctions::SubtractSparseMat(const cv::SparseMat& operand1, const cv::SparseMat& operand2, int depthTolerance, int xTolerance, int yTolerance)
{	
	//diff(x, y, z) = |operand2(x,y,z) - operand1(x,y,z)|
	//				= 1 if |1 - 0| or |0 - 1|
	//				= 0 if |0 - 0| or |1 - 1|

	//init difference:
	cv::SparseMat difference = cv::SparseMat(this->dims, this->matrixSize, operand2.type());

	//go through nonzero values of operand2, and set diff to 1 if operand1(x,y,z)==0		
	for (cv::SparseMatConstIterator op2It=operand2.begin(); op2It != operand2.end(); ++op2It)
	{
		const cv::SparseMat::Node* n = op2It.node();
		uchar val2 = op2It.value<uchar>();
		uchar val1 = 0;

		//search for the neighborhood of z (to handle noise from kinect)
		for (int offsetZ = -depthTolerance; offsetZ<=depthTolerance; offsetZ++)
		{    
			for (int offsetX = -xTolerance; offsetX<=xTolerance; offsetX++) //search for the neighborhood of x
			{
				for (int offsetY = -yTolerance; offsetY<=yTolerance; offsetY++) //search for the neighborhood of y
				{
					//WARNING: do not replace following idx[0-1-2] with idx[X-Y-Z]
					uchar temp = operand1.value<uchar>(n->idx[0]+offsetX, n->idx[1]+offsetY, n->idx[2]+offsetZ);
					if (temp > 0)
					{
						val1 = temp;
						//cout << "Found one in the neighborhood! (1)" << endl;
						break;
					}
				}
			}
		}

		if (val1 <= 0)
			difference.ref<uchar>(n->idx) = val2;
	}
	//go through nonzero values of operand1, and set diff to 1 if operand2(x,y,z)==0		
	for (cv::SparseMatConstIterator op1It=operand1.begin(); op1It != operand1.end(); ++op1It)
	{
		const cv::SparseMat::Node* n = op1It.node();
		uchar val1 = op1It.value<uchar>();
		uchar val2 = 0;	

		//search for the neighborhood of z (to handle noise from kinect)
		for (int offset = -depthTolerance; offset<=depthTolerance; offset++)
		{
			for (int offsetX = -xTolerance; offsetX<=xTolerance; offsetX++) //search for the neighborhood of x
			{
				for (int offsetY = -yTolerance; offsetY<=yTolerance; offsetY++) //search for the neighborhood of y
				{
					//WARNING: do not replace following idx[0-1-2] with idx[X-Y-Z]
					uchar temp = operand2.value<uchar>(n->idx[0]+offsetX, n->idx[1]+offsetY, n->idx[2]+offset);
					if (temp > 0)
					{
						val2 = operand2.value<uchar>(n->idx[0], n->idx[1], n->idx[2]+offset);
						//cout << "Found one in the neighborhood! (2)" << endl;
						break;
					}
				}
			}
		}

		if (val2 <= 0)
			difference.ref<uchar>(n->idx) = val1;
	}

	//all of the remaining points of difference should be equal to 0.

	return difference;
}


vector<cv::SparseMat> VmtFunctions::CalculateVolumeObjectDifferencesSparse(const vector<cv::SparseMat>& volumeObjects, int depthTolerance)
{
	cv::SparseMat operand1, operand2;
	vector<cv::SparseMat> volumeObjectDifferences;
	short counter = 2;	

    for (std::vector<cv::SparseMat>::const_iterator it = volumeObjects.begin() ; it != volumeObjects.end(); ++it)
	{
        if (it == volumeObjects.begin()) //skip the first one
		{	
			it->assignTo(operand2);
			continue;
		}

		operand1.release();
		operand2.copyTo(operand1);
		operand2.release();
		it->assignTo(operand2);		

		// now operand2 is current volume object, operand1 is previous volume object
		//diff(x, y, z) = |operand2(x,y,z) - operand1(x,y,z)|
		//				= 1 if |1 - 0| or |0 - 1|
		//				= 0 if |0 - 0| or |1 - 1|

		//init difference:
		cv::SparseMat difference = cv::SparseMat(this->dims, this->matrixSize, operand2.type());

		//go through nonzero values of operand2, and set diff to 1 if operand1(x,y,z)==0		
		for (cv::SparseMatConstIterator op2It=operand2.begin(); op2It != operand2.end(); ++op2It)
		{
			const cv::SparseMat::Node* n = op2It.node();
			uchar val2 = op2It.value<uchar>();
			uchar val1 = 0;

			//search for the neighborhood of z (to handle noise from kinect)
			for (int offset = -depthTolerance; offset<depthTolerance; offset++)
			{
				//WARNING: do not replace following idx[0-1-2] with idx[X-Y-Z]
				uchar temp = operand1.value<uchar>(n->idx[0], n->idx[1], n->idx[2]+offset);
				if (temp > 0)
				{
					val1 = temp;
					//cout << "Found one in the neighborhood! (1)" << endl;
					break;
				}
			}

			if (val1 <= 0)
				difference.ref<uchar>(n->idx) = val2;
		}
		//go through nonzero values of operand1, and set diff to 1 if operand2(x,y,z)==0		
		for (cv::SparseMatConstIterator op1It=operand1.begin(); op1It != operand1.end(); ++op1It)
		{
			const cv::SparseMat::Node* n = op1It.node();
			uchar val1 = op1It.value<uchar>();
			uchar val2 = 0;	

			//search for the neighborhood of z (to handle noise from kinect)
			for (int offset = -depthTolerance; offset<depthTolerance; offset++)
			{
				//WARNING: do not replace following idx[0-1-2] with idx[X-Y-Z]
				uchar temp = operand2.value<uchar>(n->idx[0], n->idx[1], n->idx[2]+offset);
				if (temp > 0)
				{
					val2 = operand2.value<uchar>(n->idx[0], n->idx[1], n->idx[2]+offset);
					//cout << "Found one in the neighborhood! (2)" << endl;
					break;
				}
			}

			if (val2 <= 0)
				difference.ref<uchar>(n->idx) = val1;
		}

		//all of the remaining points of difference should be equal to 0.

		cout << "Difference calculated [" << counter << ", " << counter - 1 << "], nonzero: " << (int)difference.nzcount() << endl;
		volumeObjectDifferences.push_back(difference);		
		counter++;
		difference.release();
	}
	operand1.release();
	operand2.release();

	return volumeObjectDifferences;
}

int VmtFunctions::MagnitudeOfMotion(const cv::SparseMat& sparseMat)
{
	return (int)sparseMat.nzcount();
}

double VmtFunctions::AttenuationConstantForAnAction(const vector<cv::SparseMat>& volumeObjects)
{
	double attConst = 0.0;
	int sumOfMagnitutedOfMotion = 0;
	int curMagnituteOfMotion = 0;
	int counter = 1;

    for(vector<cv::SparseMat>::const_iterator it=volumeObjects.begin(); it != volumeObjects.end(); ++it)
	{
		curMagnituteOfMotion = MagnitudeOfMotion((*it));
		sumOfMagnitutedOfMotion += curMagnituteOfMotion;
	}
	attConst = (double)(I_MAX - 1) / (double)sumOfMagnitutedOfMotion;

	return attConst;
}

cv::SparseMat VmtFunctions::ConstructVMT(const vector<cv::SparseMat>& volumeObjectDifferences)
{
	vector<cv::SparseMat> vmtList;
	cv::SparseMat curVmt;
	cv::SparseMat prevVmt;
	//Formulation:
	//VMT(x, y, z, t) = 255 if volumeObjectDifference(x, y, z, t) == 1
	//				  = max(0, VMT(x, y, z, t-1)-(attenuationConstant)*(magnitudeOfMotion(t)), otherwise

	//calculate attenuation constant for the set:
	double attConst = AttenuationConstantForAnAction(volumeObjectDifferences);
	int curMagnituteOfMotion = 0;

	//repeat for all volume differences:
    for(vector<cv::SparseMat>::const_iterator deltaIt=volumeObjectDifferences.begin(); deltaIt != volumeObjectDifferences.end(); ++deltaIt)
	{		
		curVmt = cv::SparseMat(this->dims, deltaIt->size(), deltaIt->type()); //create a sparse matrix
		curMagnituteOfMotion = MagnitudeOfMotion(*deltaIt); //calculate magnitute of motion of current volume difference

		//for all delta, except the first one:
        if (deltaIt != volumeObjectDifferences.begin())
		{
			//take the previous VMT
			prevVmt = vmtList.back();
			double constant = attConst*curMagnituteOfMotion;
			//for all nonzero values of previous VMT
			for(cv::SparseMatConstIterator pit = prevVmt.begin(); pit != prevVmt.end(); ++pit)
			{
				const cv::SparseMat::Node* n = pit.node();

				if (deltaIt->value<uchar>(n->idx) <= 0) //if difference is zero at that point
				{
					//and if value is bigger than [attenuation constant times magnitute of motion]
					uchar newValue = pit.value<uchar>() - (uchar)(constant);
					if (newValue > 0)
					{
						curVmt.ref<uchar>(n->idx) = newValue;
					} 
				}
			}
		}

		//for all, set points of current VMT to I_MAX where difference is non-zero:
		for(cv::SparseMatConstIterator dit = deltaIt->begin(); dit != deltaIt->end(); ++dit)
		{
			const cv::SparseMat::Node* n = dit.node();
			curVmt.ref<uchar>(n->idx) = I_MAX;
		}		
		vmtList.push_back(curVmt);
	}

	return vmtList.back();
}

vector<cv::SparseMat> VmtFunctions::ConstructVMTs(const vector<cv::SparseMat>& volumeObjectDifferences)
{
	vector<cv::SparseMat> vmtList;
	cv::SparseMat curVmt;
	cv::SparseMat prevVmt;
	//Formulation:
	//VMT(x, y, z, t) = 255 if volumeObjectDifference(x, y, z, t) == 1
	//				  = max(0, VMT(x, y, z, t-1)-(attenuationConstant)*(magnitudeOfMotion(t)), otherwise

	//calculate attenuation constant for the set:
	double attConst = AttenuationConstantForAnAction(volumeObjectDifferences);
	int curMagnituteOfMotion = 0;

	//repeat for all volume differences:
    for(vector<cv::SparseMat>::const_iterator deltaIt=volumeObjectDifferences.begin(); deltaIt != volumeObjectDifferences.end(); ++deltaIt)
	{		
		curVmt = cv::SparseMat(this->dims, deltaIt->size(), deltaIt->type()); //create a sparse matrix
		curMagnituteOfMotion = MagnitudeOfMotion(*deltaIt); //calculate magnitute of motion of current volume difference

		//for all delta, except the first one:
        if (deltaIt != volumeObjectDifferences.begin())
		{
			//take the previous VMT
			prevVmt = vmtList.back();
			double constant = attConst*curMagnituteOfMotion;
			//for all nonzero values of previous VMT
			for(cv::SparseMatConstIterator pit = prevVmt.begin(); pit != prevVmt.end(); ++pit)
			{
				const cv::SparseMat::Node* n = pit.node();

				if (deltaIt->value<uchar>(n->idx) <= 0) //if difference is zero at that point
				{
					//and if value is bigger than attenuation constant times magnitute of motion
					uchar newValue = pit.value<uchar>() - (uchar)(constant);
					if (newValue > 0)
					{
						curVmt.ref<uchar>(n->idx) = newValue;
					} 
				}
			}
		}

		//for all, set points of current VMT to I_MAX where difference is non-zero:
		for(cv::SparseMatConstIterator dit = deltaIt->begin(); dit != deltaIt->end(); ++dit)
		{
			const cv::SparseMat::Node* n = dit.node();
			curVmt.ref<uchar>(n->idx) = I_MAX;
		}
		cout << "VMT constructed. nonzero: " << (int)curVmt.nzcount() << endl;
		vmtList.push_back(curVmt);
	}

	return vmtList;
}

cv::SparseMat VmtFunctions::CalculateD_Old(cv::SparseMat lastVolumeObject, cv::SparseMat firstVolumeObject) //equation (8) from the paper
{
	cv::SparseMat d_Old(3, lastVolumeObject.size(), lastVolumeObject.type());

	//only condition where d_old is non-zero is first>0 and last=0 (see below)
	for (cv::SparseMatConstIterator opIt=lastVolumeObject.begin(); opIt != lastVolumeObject.end(); ++opIt)
	{
		const cv::SparseMat::Node* n = opIt.node();
		uchar lastVal = opIt.value<uchar>();
		uchar firstVal = firstVolumeObject.value<uchar>(n->idx);
		if (firstVal - lastVal > 0)
			d_Old.ref<uchar>(n->idx) = firstVal - lastVal;
	}

	return d_Old;

	//d_Old(x, y, z) = max(0, first(x,y,z) - last(x,y,z)
	//			0	 =			0				0
	//			0	 =			1				1
	//			0    =			0				1
	//			1	 =			1				0

	/*cv::Mat firstMat;
	cv::Mat lastMat;
	cv::Mat difference;
	cv::Mat zeros;
	cv::Mat max;

	lastVolumeObject.convertTo(lastMat, lastVolumeObject.type());
	firstVolumeObject.convertTo(firstMat, firstVolumeObject.type());	
	zeros = cv::Mat(firstMat.dims, firstMat.size, firstMat.type(), 0);		

	cv::subtract(firstMat, lastMat, difference);
	firstMat.release();
	lastMat.release();

	max = cv::max(difference, zeros);
	difference.release();
	zeros.release();


	cv::SparseMat sparseMax(max);
	max.release();
	return sparseMax;*/
}

cv::SparseMat VmtFunctions::CalculateD_New(cv::SparseMat lastVolumeObject, cv::SparseMat firstVolumeObject) //equation (7) from the paper
{
	cv::SparseMat d_New(3, lastVolumeObject.size(), lastVolumeObject.type());

	//only condition where d_old is non-zero is first>0 and last=0 (see below)
	for (cv::SparseMatConstIterator opIt=lastVolumeObject.begin(); opIt != lastVolumeObject.end(); ++opIt)
	{
		const cv::SparseMat::Node* n = opIt.node();
		uchar lastVal = opIt.value<uchar>();
		uchar firstVal = firstVolumeObject.value<uchar>(n->idx);
		if (lastVal - firstVal > 0)
			d_New.ref<uchar>(n->idx) = lastVal - firstVal;
	}

	return d_New;

	//d_New(x, y, z) = max(0, last(x,y,z) - first(x,y,z)
	//			0	 =			0				0
	//			0	 =			1				1
	//			0    =			0				1
	//			1	 =			1				0
}

cv::Vec3i VmtFunctions::CalculateMomentVector(cv::SparseMat volumeObjectSparse) //equation (9) from the paper
{
	int sumOfAllElements = (int)volumeObjectSparse.nzcount(); //sum of all elements of a volumeObject is equal to number of nonzero (=1) elements

	cv::Vec3i sumVector(0, 0, 0);
	for(cv::SparseMatConstIterator it = volumeObjectSparse.begin(); it != volumeObjectSparse.end(); ++it)
	{		
		const cv::SparseMat::Node* n = it.node();		
		cv::Vec3i currentVector(n->idx[0], n->idx[1], n->idx[2]);		
		sumVector += currentVector;		
	}

	cv::Vec3i momentVector((int)(sumVector.val[0]/sumOfAllElements), (int)(sumVector.val[1]/sumOfAllElements), (int)(sumVector.val[2]/sumOfAllElements));

	return momentVector;
}

double VmtFunctions::CalculateAlpha(cv::Vec3i motionVector) //equation (14) from the paper
{
	//Indices: y=0, x=1, z=2	
	//norm of projection of vector (a, b, c) to axis y is 'a' (because index of y=0)
	int y = motionVector.val[0],
		z = motionVector.val[2];
	return 1.0/cos(-(HelperFunctions::sgn(y))*(HelperFunctions::sgn(z))*((double)y / (double)(y - z)));	 
}

double VmtFunctions::CalculateBeta(cv::Vec3i motionVector) //equation (15) from the paper
{
	//Indices: y=0, x=1, z=2
	//norm of projection of vector (a, b, c) to axis y is 'a' (because index of y=0)
	int x = motionVector.val[1],
		z = motionVector.val[2];
	return 2.0/CV_PI - 1.0/cos(-(HelperFunctions::sgn(z))*HelperFunctions::sgn(x)*((double)z / (double)(z-x)));
}

double VmtFunctions::CalculateTheta(cv::Vec3i motionVector) //equation (16) from the paper
{
	//Indices: y=0, x=1, z=2
	//norm of projection of vector (a, b, c) to axis y is 'a' (because index of y=0)
	int x = motionVector.val[1],
		y = motionVector.val[0];

	return 1.0/cos(-(HelperFunctions::sgn(x))*HelperFunctions::sgn(y)*((double)x / (double)(x-y)));
}

cv::Matx33d VmtFunctions::CalculateRotationX_alpha(double alpha) //equation (11) from the paper
{
	return 
		cv::Matx33d(	1.0,		0.0,			0.0,
		0.0,		cos(alpha),		sin(alpha), 
		0.0,		-sin(alpha),	cos(alpha)		);

}

cv::Matx33d VmtFunctions::CalculateRotationY_beta(double beta) //equation (12) from the paper
{
	return 
		cv::Matx33d(	cos(beta),	0.0,		-sin(beta),
		0.0,		1.0,		0.0,
		sin(beta),	0.0,		cos(beta)		);
}

cv::Matx33d VmtFunctions::CalculateRotationZ_theta(double theta) //equation (13) from the paper
{
	return 
		cv::Matx33d(	cos(theta),		sin(theta),		0.0,
		-sin(theta),	cos(theta),		0.0,
		0.0,			0.0,			1.0		);
}

cv::SparseMat VmtFunctions::RotateVMT(const cv::SparseMat& vmt, const cv::Matx33d& rotationMatrix)
{	
	int newSizes[] = {5000, 5000, 5000}; //FIXME
	cv::SparseMat rotatedVmt(3, vmt.size(), vmt.type());
	cv::SparseMat shiftedRotatedVmt(3, vmt.size(), vmt.type());

	cv::Matx13d curVec;
	cv::Matx13d rotVec;
	int idx[3];
	int minY = INT_MAX, /*maxY = INT_MIN,*/ minX = INT_MAX/*, maxX = INT_MIN, minZ = INT_MAX, maxZ = INT_MIN*/;
	int translationX = 0, translationY = 0;

	for(cv::SparseMatConstIterator it = vmt.begin(); it != vmt.end(); ++it)
	{
		//for each point
		const cv::SparseMat::Node* n = it.node();	
		//create a 3d vector:
		curVec = cv::Matx13d(n->idx[0], n->idx[1], n->idx[2]);
		//rotate it with the matrix
		rotVec = curVec * rotationMatrix;

		//find min-max of rotated points
		minY = min(minY, idx[0]);
		//maxY = max(maxY, idx[0]);
		minX = min(minX, idx[1]);
		//maxX = max(maxX, idx[1]);
		//minZ = min(minZ, idx[2]);
		//maxZ = max(maxZ, idx[2]);		

		idx[0] = (int)rotVec.val[0];		
		idx[1] = (int)rotVec.val[1]; 
		idx[2] = (int)rotVec.val[2];
		//set rotated point to rotated VMT		
		rotatedVmt.ref<uchar>(idx) = it.value<uchar>();

	} 
	//we try to shift the minimum point to 0
	translationX = -minX;
	translationY = -minY;
	cout << "translation x: " << translationX << ", translation y: " << translationY << endl;

	//shift the rotated VMT into the image frame:
	for(cv::SparseMatConstIterator rit = rotatedVmt.begin(); rit != rotatedVmt.end(); ++rit)
	{
		const cv::SparseMat::Node* n = rit.node();
		idx[0] = n->idx[0] + translationY;		
		idx[1] = n->idx[1] + translationX;
		idx[2] = n->idx[2];
		shiftedRotatedVmt.ref<uchar>(idx) = rit.value<uchar>();
	}

	rotatedVmt.release();

	//cout << "[" << minY << ", " << maxY << "], [" << minX << ", " << maxX << "], [" << minZ << ", " << maxZ << "]\n";

	return shiftedRotatedVmt;
}

cv::Mat VmtFunctions::ProjectVMTOntoXY(const cv::SparseMat& vmt)
{
	//cv::Mat projectedMatrix(480, 640, vmt.type()); //create an empty matrix
	//FIXME:
	cv::Mat projectedMatrix(1000, 800, vmt.type());
    projectedMatrix = cv::Scalar(0); //set all values to zero


	for(cv::SparseMatConstIterator it = vmt.begin(); it != vmt.end(); ++it) 
	{
		//for each point in 3D
		const cv::SparseMat::Node* n = it.node();		
		//FIXME
		if (n->idx[0] >=0 && n->idx[0] < 800 && n->idx[1]>= 0 && n->idx[1] < 800)
		{
			projectedMatrix.at<uchar>(n->idx[0], n->idx[1]) 
				= max(projectedMatrix.at<uchar>(n->idx[0], n->idx[1]), it.value<uchar>());					
		}
	}

	return projectedMatrix;
}

//other methods
void VmtFunctions::Print3x3Matrix(const cv::Matx33d& mat)
{
	cv::Mat M(mat);
	for(int i=0; i<3; ++i)
	{	
		for (int j=0; j<3; ++j)
		{
			cout << M.at<double>(i, j) << "\t";
		}
		cout << endl;
	}


}

void VmtFunctions::Print3DSparseMatrix(cv::SparseMat sparse_mat)
{		
	cv::Mat denseMat;
	sparse_mat.convertTo(denseMat, CV_8UC1);

	int idx[3];	
	for (int i=0; i<this->matrixSize[Y] ; ++i)
	{
		for (int j=0; j<this->matrixSize[X] ; ++j)
		{
			for (int k=0; k<this->matrixSize[Z] ; ++k)
			{
				idx[0] = i; idx[1] = j; idx[2] = k;
				uchar value = denseMat.at<uchar>(idx);
				if (value > 0)
				{
					cout << "(" << i << ",\t" << j << ",\t" << k << "):\t" << value << endl; 
				}
			}
		}
	}
}

void VmtFunctions::Save3DSparseMatrix(cv::SparseMat sparse_mat, string filePath)
{
    QFile myfile(QString::fromStdString(filePath));
    if (!myfile.open(QFile::ReadWrite))
    {
        cout << "ERROR!" << endl;
        return;
    }

    QTextStream stream(&myfile);

	cv::Mat denseMat;
	sparse_mat.convertTo(denseMat, CV_8UC1);

	int idx[3];	
	for (int i=0; i<this->matrixSize[Y] ; ++i)
	{
		for (int j=0; j<this->matrixSize[X] ; ++j)
		{
			for (int k=0; k<this->matrixSize[Z] ; ++k)
			{
				idx[0] = i; idx[1] = j; idx[2] = k;
				uchar value = denseMat.at<uchar>(idx);
				if (value > 0)
				{
                    stream << i << ";" << j << ";" << k << ";" << value << endl;
				}
			}
		}
	}

    //stream.flush();
	myfile.close();
}

void VmtFunctions::Save3DMatrix(cv::Mat mat, string filePath)
{
    QFile myfile(QString::fromStdString(filePath));
    if (!myfile.open(QFile::ReadWrite))
    {
        cout << "ERROR!" << endl;
        return;
    }

    QTextStream stream(&myfile);

	int idx[3];	
	for (int i=0; i<this->matrixSize[Y] ; ++i)
	{
		for (int j=0; j<this->matrixSize[X] ; ++j)
		{
			for (int k=0; k<this->matrixSize[Z] ; ++k)
			{
				idx[0] = i; idx[1] = j; idx[2] = k;
				uchar value = mat.at<uchar>(idx);
				if (value > 0)
				{
                    stream << i << ";" << j << ";" << k << ";" << value << endl;
				}
			}
		}
	}
	myfile.close();
}

