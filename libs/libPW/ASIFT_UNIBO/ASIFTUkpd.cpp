/**
*-------------------------------------------------
*  Copyright 2016 by Tidop Research Group <daguilera@usal.se>
*
* This file is part of GRAPHOS - inteGRAted PHOtogrammetric Suite.
*
* GRAPHOS - inteGRAted PHOtogrammetric Suite is free software: you can redistribute
* it and/or modify it under the terms of the GNU General Public
* License as published by the Free Software Foundation, either
* version 3 of the License, or (at your option) any later version.
*
* GRAPHOS - inteGRAted PHOtogrammetric Suite is distributed in the hope that it will
* be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
* @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
*-------------------------------------------------
*/
#include "ASIFTUkpd.h"
#include "MSD/ProcessedImageData.h"
#include "ASIFT_UNIBO/asiftdetector.h"
#include <QFileInfo>
#include <QDir>
using namespace PW;
using namespace cv;

void writeDescsInAscii(std::string fileName,std::vector< KeyPoint > &keypoints, std::vector<std::vector<int>> &descriptors, float rsf){
    FILE* fileOut;
    errno_t err;

    float irsf = 1 / rsf;

    char ff[1024];
    strncpy(ff, fileName.c_str(), sizeof(ff));
    ff[sizeof(ff) - 1] = 0;

    int rows = keypoints.size();
    int cols = 128;

    const char * fName = (const char *)ff;
    err  = fopen_s( &fileOut, fName, "w" );

    if (err==0){
        fprintf(fileOut,"%d %d\n",rows,cols);

        for(int i=0;i<rows;i++)
        {
            fprintf(fileOut,"%f %f %f %f\n",keypoints[i].pt.y * irsf,keypoints[i].pt.x * irsf,keypoints[i].size *irsf , keypoints[i].angle);
            for(int j=0;j<cols;j++)
            {
                fprintf(fileOut,"%d ",descriptors.at(i).at(j));
            }
            fprintf(fileOut,"\n");

        }
        fclose(fileOut);
    }
}


ASIFTUkpd::ASIFTUkpd(QString inputFile, QString outputDir, ProcessedImageData *metadata,int imageResizeValue,int peakTh,float edgeTh, int octaves,int maxTiepoints,int tilts):
    mInputFile(inputFile),
    mOtputDir(outputDir),
    mImageMetadata(metadata),
    ASIFTImageResizeValue(imageResizeValue),
    mPeakTh(peakTh),
    mEdgeTh(edgeTh),
    mOctaves(octaves),
    mMaxTiepoints(maxTiepoints),
    mtilts(tilts)
{

}

ASIFTUkpd::~ASIFTUkpd()
{

}

void ASIFTUkpd::run(){
    QFileInfo fileInfo(mInputFile);
    QString *stdData = new QString();

    stdData->clear();
    stdData->append("Starting ASIFT keypoint detection for image:  ");
    stdData->append(fileInfo.fileName());
    emit newStdData(stdData->toAscii());

    QString matchesFolder = mOtputDir;

    cv::String cvImageFileName = fileInfo.absoluteFilePath().toStdString();

    cv::Mat cvImg = cv::imread(cvImageFileName, CV_LOAD_IMAGE_GRAYSCALE);

    if (ASIFTImageResizeValue!=-1 && (cvImg.size[1]> ASIFTImageResizeValue || cvImg.size[0]> ASIFTImageResizeValue)) {

        if (cvImg.size[1]>cvImg.size[0] && cvImg.size[1]>ASIFTImageResizeValue) {
            //Scale
            int newImageHeight = (int)(cvImg.rows*ASIFTImageResizeValue/cvImg.cols);

            Size newSize(ASIFTImageResizeValue,newImageHeight);

            cv::resize(cvImg,cvImg,newSize,INTER_NEAREST   ); //resize image

            stdData->clear();
            stdData->append("Image ");
            stdData->append(fileInfo.fileName());
            stdData->append(" downsampled.");
            emit newStdData(stdData->toAscii());
        }else if (cvImg.size[0]>cvImg.size[1] && cvImg.size[0]>ASIFTImageResizeValue) {
            //Scale

            int newImageWidth = (int)(cvImg.cols*ASIFTImageResizeValue/cvImg.rows);

            Size newSize(newImageWidth,ASIFTImageResizeValue);

            cv::resize(cvImg,cvImg,newSize,INTER_NEAREST   ); //resize image

            stdData->clear();
            stdData->append("Image ");
            stdData->append(fileInfo.fileName());
            stdData->append(" downlampled.");
            emit newStdData(stdData->toAscii());
        }
    }

    cv::Mat kpsImg;
    cv::Mat auxImage;
    std::vector<cv::KeyPoint> cvImg_keyPoints;
    cvImg_keyPoints.clear();

    stdData->clear();
    stdData->append("Searching keypoints for image ");
    stdData->append(fileInfo.fileName());
    emit newStdData(stdData->toAscii());


    std::vector< std::vector< int > > cvDescriptors;
    ASiftDetector detector(mPeakTh,mEdgeTh,mOctaves,mtilts);

    detector.computeAsift(cvImg,cvImg_keyPoints,cvDescriptors,mMaxTiepoints);

    //Convert vector of vectors to cv::Mat

    cv::Mat cvDescriptorsConverted(cvDescriptors.size(), cvDescriptors.at(0).size(), CV_32F);
    for(int i=0; i<cvDescriptorsConverted.rows; ++i)
         for(int j=0; j<cvDescriptorsConverted.cols; ++j){
             cvDescriptorsConverted.at<float>(i, j) = cvDescriptors.at(i).at(j);
         }

    emit statusChangedNext();

    stdData->clear();
    stdData->append(QString::number(cvImg_keyPoints.size()));
    stdData->append(" keypoints found for image ");
    stdData->append(fileInfo.fileName());
    emit newStdData(stdData->toAscii());

    emit statusChangedNext();

    mImageMetadata->setTiepoints(cvImg_keyPoints);
    mImageMetadata->setDescriptors(cvDescriptorsConverted);
    QFileInfo inputFileInfo (mInputFile);
    QString imageFileName = inputFileInfo.fileName();
    mImageMetadata->setImageName(imageFileName);

    stdData->clear();
    stdData->append("Image ");
    stdData->append(fileInfo.fileName());
    stdData->append(" processed successfull.");
    emit newStdData(stdData->toAscii());
}
