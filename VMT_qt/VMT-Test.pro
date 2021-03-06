#-------------------------------------------------
#
# Project created by QtCreator 2014-02-05T22:38:41
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = VMT-Test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    HelperFunctions.cpp \
    PointCloudFunctions.cpp \
    VmtFunctions.cpp

OTHER_FILES += \
    VMT-Test.pro.user

HEADERS += \
    HelperFunctions.h \
    PointCloudFunctions.h \
    VmtFunctions.h

INCLUDEPATH += /usr/include/pcl-1.7
INCLUDEPATH += /usr/include/eigen3

LIBS += -lpcl_common -lpcl_visualization -lpcl_filters -lpcl_io \
-lboost_system \
-lopencv_core \
-lopencv_imgproc \
-lopencv_highgui \
-lopencv_ml \
-lopencv_video \
-lopencv_features2d \
-lopencv_calib3d \
-lopencv_objdetect \
-lopencv_contrib \
-lopencv_legacy \
-lopencv_flann
