/*
 * GLUT Demo project for slicing stl file
 *
 * Written by
 *
 * This program is test harness for the sphere, cone
 * and torus shapes in GLUT
 * Mathematical support:
 *
 * Spinning wireframe and smooth shaded shapes are
 * displayed until the ESC or q key is pressed.  The
 * number of geometry stacks and slices can be adjusted
 * using the + and - keys.
 * Autors:
 */

#define maxInnerRandPoint 50   /// Количество рандомных точек внутри констура
#define GridSize 5    /// Величина ячейки внутренней сетки для тринагуляции

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <math.h>
#include <windows.h>

using namespace std;

///Структура определяющая точку в 3D
struct point{
    float X;
    float Y;
    float Z;
};

///Структура определеяющая один треугольик в модели
struct triangle{
    point Normal;
    point p[3];
};

///Структура определяющая точку в 2D
struct point2D{
    float x;
    float y;
};

///Структура хранящая ограничивающий многоугольник для процесса создания ячеек вороного
struct poligone{
    vector <point2D> StackCorners;
};

///Базовая точка, начало координат
point2D TempBasePoint;

///Массив хранящий каждый треугольник самой модели
vector <triangle>triangleBase;

///Переменные хранящие положение и габариты модели
float GabariteMaxX;
float GabariteMinX;
float GabariteMaxY;
float GabariteMinY;
float GabariteMaxZ;
float GabariteMinZ;

///Массивы для обработки и хранения линий в процессе слайсинга и их ID
vector <point>pointSeparation;
vector <int>pointSeparationID;
vector <point>OutLineSeparation;
vector <int>OutLineSeparationID;
vector <vector <point> > OutLineLoop;
vector <vector <int> > OutLineLoopID;
int countLoops;
vector <point>InnerPoints;
vector <point2D> MeabyPoint;
vector <poligone> PoligoneBase;

///Глобальные переменные для работы со сценой
float StartAngleCamRotateX;
float StartAngleCamRotateY;
float DeltaAngleCamRotateX;
float DeltaAngleCamRotateY;
float angleCamRotateX;
float angleCamRotateY;
float zoomScene;
float SlicerHeight;
int offsetUpForLinePerimetr;
bool HideSolid;
float LayerHeight;

///Фунция вычисляющая координаты точки пересечения двух отрезков
bool Intersect2Line(float ax1, float ay1, float ax2, float ay2, float bx1, float by1, float bx2, float by2){
    float v1=(bx2-bx1)*(ay1-by1)-(by2-by1)*(ax1-bx1);
    float v2=(bx2-bx1)*(ay2-by1)-(by2-by1)*(ax2-bx1);
    float v3=(ax2-ax1)*(by1-ay1)-(ay2-ay1)*(bx1-ax1);
    float v4=(ax2-ax1)*(by2-ay1)-(ay2-ay1)*(bx2-ax1);
    return (v1*v2<0) && (v3*v4<0);
}

///Выдает угол наклона вектора от математического угла 0
float atanTrueDegree(float x, float y){
    return (atan2(y,x)*180/M_PI)<0?((atan2(y,x)*180/M_PI)+360):(atan2(y,x)*180/M_PI);
}

///Угол между отрезками
float FindAngleOtrezok(float x1, float y1, float x2, float y2){
    return (atan2(y2-y1,x2-x1)*180/M_PI)<0?((atan2(y2-y1,x2-x1)*180/M_PI)+360):(atan2(y2-y1,x2-x1)*180/M_PI);
}

///Вычиляет точку пересечения линии и отрезка
bool IntersectLine2Otrezok(float lineP1x, float lineP1y, float lineP2x, float lineP2y, float otrP1x, float otrP1y, float otrP2x, float otrP2y){
    float a=FindAngleOtrezok(lineP1x, lineP1y, lineP2x, lineP2y)-FindAngleOtrezok(lineP1x, lineP1y, otrP1x, otrP1y);
    float b=FindAngleOtrezok(lineP1x, lineP1y, lineP2x, lineP2y)-FindAngleOtrezok(lineP1x, lineP1y, otrP2x, otrP2y);
    float c=FindAngleOtrezok(lineP2x, lineP2y, lineP1x, lineP1y)-FindAngleOtrezok(lineP2x, lineP2y, otrP1x, otrP1y);
    float d=FindAngleOtrezok(lineP2x, lineP2y, lineP1x, lineP1y)-FindAngleOtrezok(lineP2x, lineP2y, otrP2x, otrP2y);
    //cout<<"a="<<a<<" b="<<b<<" c="<<c<<" d="<<d<<endl;
    ///Позволяет сказать пересекаются ли отрезок и линия
    if (((c>0 && d<0)||(c<0 && d>0))||((a>0 && b<0)||(a<0 && b>0))) {
        if ((a<0.001&&a>-0.001)||(b<0.001&&b>-0.001)||(c<0.001&&c>-0.001)||(d<0.001&&d>-0.001)){
            //cout<<"Intersept Danger"<<endl;
            return false;
        }
        //cout<<"Intersept TRUE"<<endl;
        return true;
    }
    //cout<<"Intersept False"<<endl;
    return false;
}

///Вычилсяет положение точки перпендикулярой к заданному отрезку в заданной точке
void GetPerpendicular(float p1x,float p1y,float p2x,float p2y, float &np1x, float &np1y, float &np2x, float &np2y){
    np1x=(p1x+p2x)/2;
    np1y=(p1y+p2y)/2;
    float angle=FindAngleOtrezok(p1x,p1y,p2x,p2y);
    angle+=90;
    if (angle>360) angle-=360;
    angle=(angle/180)*M_PI;
    np2x=cos(angle)+np1x;
    np2y=sin(angle)+np1y;
}

void DeleteEqualPoints(){

}

///Функиция вычисляющая ортоцентр треугольника по трем заданным точкам. Ортоцентр - точка
bool GetCoordsCentrTreugolnika(point p1, point p2, point p3, point2D &p4){
    float S=((p1.X-p3.X)*(p2.Y-p3.Y)-(p2.X-p3.X)*(p1.Y-p3.Y))*0.5;  ///Поиск площади треуголника
    if (S==0) return false; ///В случае когда треуголник складывается в линию функция предупреждает об этом и заканчивает свою работу
    float D=2*(p1.X*(p2.Y-p3.Y)+p2.X*(p3.Y-p1.Y)+p3.X*(p1.Y-p2.Y));
    p4.x=((p1.X*p1.X+p1.Y*p1.Y)*(p2.Y-p3.Y)+(p2.X*p2.X+p2.Y*p2.Y)*(p3.Y-p1.Y)+(p3.X*p3.X+p3.Y*p3.Y)*(p1.Y-p2.Y))/D;
    p4.y=((p1.X*p1.X+p1.Y*p1.Y)*(p3.X-p2.X)+(p2.X*p2.X+p2.Y*p2.Y)*(p1.X-p3.X)+(p3.X*p3.X+p3.Y*p3.Y)*(p2.X-p1.X))/D;
    return true;
}

///Перегрузка функции сравнения точек
bool HelpFunctionForSort(point2D a, point2D b){
    if (atanTrueDegree(TempBasePoint.x-a.x,TempBasePoint.y-a.y)>atanTrueDegree(TempBasePoint.x-b.x,TempBasePoint.y-b.y)){
        return false;
    }
    return true;
}

///Создает выпуклый многоуголник по ID точки для генерации сетки вороного (занимает ~2-3Гб оперативки)
void CreatePligoneForPoint(int currentID){
    MeabyPoint.clear();
    point2D tmp;
    ///Цикл заполнения массива кандидатов точкек представляющих собой многоуголник (без фиьльтрации)
    for (int i=0;i<InnerPoints.size();i++){
        for (int j=i+1;j<InnerPoints.size();j++){
            if (i==currentID || j==currentID) continue;
            if (!GetCoordsCentrTreugolnika(InnerPoints[currentID], InnerPoints[i], InnerPoints[j], tmp)) continue;
            MeabyPoint.push_back(tmp);
        }
    }

    cout<<"Create "<<MeabyPoint.size()<<" subPoints"<<endl;
    ///Фильтрация промежуточных точек - убираю все кто не отностися к многоугольнику. Усекает все точки стоящие за выпуклым многоугольником
    for (int i=0;i<MeabyPoint.size();i++){
        for (int j=0;j<InnerPoints.size();j++){
            if (j==currentID) continue;
            float p1x=InnerPoints[currentID].X;
            float p1y=InnerPoints[currentID].Y;
            float p2x=InnerPoints[j].X;
            float p2y=InnerPoints[j].Y;
            float np1x, np1y, np2x, np2y;
            GetPerpendicular(p1x,p1y,p2x,p2y,np1x,np1y,np2x,np2y);
            ///Удаление точки стоящей за выпуклым многоугольником
            if (IntersectLine2Otrezok(np1x,np1y,np2x,np2y,p1x,p1y,MeabyPoint[i].x,MeabyPoint[i].y)){
                MeabyPoint.erase(MeabyPoint.begin()+i,MeabyPoint.begin()+i+1);
                i=0;
                j=0;
                break;
            }
        }
    }
    cout<<"After clearing "<<MeabyPoint.size()<<" subPoints"<<endl;

    TempBasePoint.x=InnerPoints[currentID].X;
    TempBasePoint.y=InnerPoints[currentID].Y;
    ///Сортировка точек после отчистки
    sort(MeabyPoint.begin(), MeabyPoint.end(),HelpFunctionForSort);

    ///Заполнение массива многокгольников сетки вороного
    PoligoneBase.clear();
    poligone tmpPoly;
    for (int i=0;i<MeabyPoint.size();i++){
        tmp.x=MeabyPoint[i].x;
        tmp.y=MeabyPoint[i].y;
        tmpPoly.StackCorners.push_back(tmp);
        //cout<<"DEGREE="<<atanTrueDegree(TempBasePoint.x-tmp.x,TempBasePoint.y-tmp.y)<<endl;
    }
    PoligoneBase.push_back(tmpPoly);
}

///Функция проверки нахождение точки внутри замкнутого контура
bool FindPointIntoLoop(float inX,float inY){
    float dAlpha=0; ///Временный угол к которому суммируется дельта от переходов по точкам
    int countIDInLoops=0;   ///Количество контуров
        ///Цикл обхода по всем точкам контура с суммированием в временный угол
        for (int i=0;i<OutLineSeparation.size()-1;i++){
            if (OutLineSeparationID[i]!=OutLineSeparationID[i+1]){
                if (fabs(dAlpha)>0.001 ) countIDInLoops++;
                dAlpha=0;
                continue;
            }
            float p1x=OutLineSeparation[i].X-inX;
            float p1y=OutLineSeparation[i].Y-inY;
            float p2x=OutLineSeparation[i+1].X-inX;
            float p2y=OutLineSeparation[i+1].Y-inY;

            float angle=atanTrueDegree(p1x,p1y)-atanTrueDegree(p2x,p2y);
            if (angle>180) angle=-(360-atanTrueDegree(p1x,p1y)+atanTrueDegree(p2x,p2y));
            if (angle<-180) angle=(360-atanTrueDegree(p2x,p2y)+atanTrueDegree(p1x,p1y));
            dAlpha+=angle;
        }
        if (fabs(dAlpha)>0.001 ) countIDInLoops++;
        if (countIDInLoops%2==1) return true;
        else return false;
}

///Функция установки множества беспорядочных точек внутри замкунтого контура
void SetInnerPointsRand(){
    InnerPoints.clear();
    point tmp;
    if (OutLineSeparation.size()<2) return;
    while (InnerPoints.size()<maxInnerRandPoint){
        tmp.X=(((float)rand()/(float)(RAND_MAX)) * fabs(GabariteMaxX-GabariteMinX))+GabariteMinX;
        tmp.Y=(((float)rand()/(float)(RAND_MAX)) * fabs(GabariteMaxY-GabariteMinY))+GabariteMinY;
        tmp.Z=SlicerHeight;
        if (FindPointIntoLoop(tmp.X,tmp.Y)) InnerPoints.push_back(tmp);
    }
}

///Функция установки множества точек в виде систематизированной сетки внутри замкунтого контура
void SetInnerPointsGrid(){
    InnerPoints.clear();
    point tmp;
    for (float dx=GabariteMinX+GridSize/2;dx<GabariteMaxX;dx+=GridSize){
        for (float dy=GabariteMinY+GridSize/2;dy<GabariteMaxY;dy+=GridSize){
            tmp.X=dx;
            tmp.Y=dy;
            tmp.Z=SlicerHeight;
            if (FindPointIntoLoop(dx,dy)) InnerPoints.push_back(tmp);
        }
    }
}

///Функция генерации одного замкнутого контура
void FindSeparateLayerOutLine(){
    ///Строчки для поготовки к работе, очистка, создание базовой точки т т.д.
    OutLineSeparation.clear();
    OutLineSeparationID.clear();
    point tempPoint;
    int tempID;
    int thisIDLine;
    tempPoint.X=pointSeparation[0].X;
    tempPoint.Y=pointSeparation[0].Y;
    tempPoint.Z=pointSeparation[0].Z;
    tempID=0;
    thisIDLine=0;
    OutLineSeparation.push_back(tempPoint);
    OutLineSeparationID.push_back(thisIDLine);
    ///Бесконечный цикл прохода по всем точкам пересечения плоскости с моделью
    while (pointSeparationID.size()>0){
        for(int i=0;i<pointSeparation.size();i++){
//            cout<<pointSeparationID[i]<<" ID "<<pointSeparation[i].X<<"X"<<tempPoint.X<<endl;
//            cout<<pointSeparationID[i]<<" ID "<<pointSeparation[i].Y<<"Y"<<tempPoint.Y<<endl;
//            cout<<pointSeparationID[i]<<" ID "<<pointSeparation[i].Z<<"Z"<<tempPoint.Z<<endl;
            if (fabs(pointSeparation[i].X-tempPoint.X)<0.001 &&
                fabs(pointSeparation[i].Y-tempPoint.Y)<0.001 &&
                fabs(pointSeparation[i].Z-tempPoint.Z)<0.001){
                    for (int j=0;j<pointSeparation.size();j++){
//                        cout<<pointSeparationID[j]<<"="<<pointSeparationID[i]<<endl;
//                        cout<<"\t"<<pointSeparation[j].X<<"="<<pointSeparation[i].X<<endl;
//                        cout<<"\t"<<pointSeparation[j].Y<<"="<<pointSeparation[i].Y<<endl;
//                        cout<<"\t"<<pointSeparation[j].Z<<"="<<pointSeparation[i].Z<<endl;
//                        system("pause");
                        if ((pointSeparation[j].X!=pointSeparation[i].X ||
                            pointSeparation[j].Y!=pointSeparation[i].Y ||
                            pointSeparation[j].Z!=pointSeparation[i].Z) &&
                            pointSeparationID[j]==pointSeparationID[i]){
//                                cout<<"ID-"<<pointSeparationID[i]<<" X"<<pointSeparation[i].X<<"="<<pointSeparation[j].X<<endl;
//                                cout<<"ID-"<<pointSeparationID[i]<<" Y"<<pointSeparation[i].Y<<"="<<pointSeparation[j].Y<<endl;
//                                cout<<"ID-"<<pointSeparationID[i]<<" Z"<<pointSeparation[i].Z<<"="<<pointSeparation[j].Z<<endl;
//                                system("pause");
                                tempPoint.X=pointSeparation[j].X;
                                tempPoint.Y=pointSeparation[j].Y;
                                tempPoint.Z=pointSeparation[j].Z;
                                OutLineSeparation.push_back(tempPoint);
                                OutLineSeparationID.push_back(thisIDLine);
                                tempID=pointSeparationID[i];
//                                for (int i=0;i<pointSeparation.size();i++){
//                                    cout<<"P#"<<i<<" ID="<<pointSeparationID[i]<<" X="<<pointSeparation[i].X<<" Y="<<pointSeparation[i].Y<<" Z="<<pointSeparation[i].Z<<endl;
//                                }
//                                system("pause");
                                ///Фильтрация получившегося констра на совпадение ID
                                for(int k=0;k<pointSeparationID.size();k++){
                                    if (pointSeparationID[k]==tempID){
                                        pointSeparation.erase(pointSeparation.begin()+k,pointSeparation.begin()+k+1);
                                        pointSeparationID.erase(pointSeparationID.begin()+k,pointSeparationID.begin()+k+1);
                                        break;
                                    }
                                }
                                for(int k=0;k<pointSeparationID.size();k++){
                                    if (pointSeparationID[k]==tempID){
                                        pointSeparation.erase(pointSeparation.begin()+k,pointSeparation.begin()+k+1);
                                        pointSeparationID.erase(pointSeparationID.begin()+k,pointSeparationID.begin()+k+1);
                                        break;
                                    }
                                }
//                                for (int i=0;i<pointSeparation.size();i++){
//                                    cout<<"P#"<<i<<" ID="<<pointSeparationID[i]<<" X="<<pointSeparation[i].X<<" Y="<<pointSeparation[i].Y<<" Z="<<pointSeparation[i].Z<<endl;
//                                }
//                                cout<<"NewTemp X="<<tempPoint.X<<endl;
//                                cout<<"NewTemp Y="<<tempPoint.Y<<endl;
//                                cout<<"NewTemp Z="<<tempPoint.Z<<endl;
//                                cout<<"NewTemp IDLoop="<<thisIDLine<<endl;
//                                system("pause");
                                break;
                        }
                    }
                    break;
            }
//            system("pause");
            ///Оплетка поиска количества контуров
            if (i==pointSeparation.size()-1){
                tempPoint.X=pointSeparation[0].X;
                tempPoint.Y=pointSeparation[0].Y;
                tempPoint.Z=pointSeparation[0].Z;
                tempID=0;
                OutLineSeparation.push_back(tempPoint);
                OutLineSeparationID.push_back(thisIDLine+1);
                thisIDLine++;
               // cout<<"Detected end loop #"<<thisIDLine<<endl;
            }
        }
    }

    ///Дополнительная фильтрация массива контура для удаления лишних вершин в узле которой линия не изгибается
    int thisPointStartLine=0;
    while (thisPointStartLine<OutLineSeparation.size()-2) {
        if (OutLineSeparationID[thisPointStartLine]!=OutLineSeparationID[thisPointStartLine+2]) {
            thisPointStartLine+=2;
        }
        float dx1=fabs(OutLineSeparation[thisPointStartLine].X-OutLineSeparation[thisPointStartLine+1].X);
        float dy1=fabs(OutLineSeparation[thisPointStartLine].Y-OutLineSeparation[thisPointStartLine+1].Y);
        float dx2=fabs(OutLineSeparation[thisPointStartLine+1].X-OutLineSeparation[thisPointStartLine+2].X);
        float dy2=fabs(OutLineSeparation[thisPointStartLine+1].Y-OutLineSeparation[thisPointStartLine+2].Y);
        //cout<<"1Angle devision is="<<(dx1/dy1)<<" "<<(dx2/dy2)<<endl;
        //cout<<"2Angle devision is="<<(dy1/dx1)<<" "<<(dy2/dx2)<<endl;
        if ((dx1/dy1)==(dx2/dy2)){
            //cout<<"angleEqual in Line#"<<thisPointStartLine<<"-"<<thisPointStartLine+1<<" to "<<thisPointStartLine+1<<"-"<<thisPointStartLine+2<<endl;
            OutLineSeparation[thisPointStartLine+1].X=OutLineSeparation[thisPointStartLine+2].X;
            OutLineSeparation[thisPointStartLine+1].Y=OutLineSeparation[thisPointStartLine+2].Y;
            OutLineSeparation[thisPointStartLine+1].Z=OutLineSeparation[thisPointStartLine+2].Z;
            OutLineSeparation.erase(OutLineSeparation.begin()+thisPointStartLine+2,OutLineSeparation.begin()+thisPointStartLine+3);
            OutLineSeparationID.erase(OutLineSeparationID.begin()+thisPointStartLine+2,OutLineSeparationID.begin()+thisPointStartLine+3);
            thisPointStartLine=0;
        }
        else thisPointStartLine+=1;
    }

    offsetUpForLinePerimetr=OutLineSeparation.size();
    countLoops=thisIDLine+1;
    //cout<<"Line loops count = "<<thisIDLine+1<<endl;
}

///Старая реализация функции... Может еще пригодится нектороые моменты
//void FindSeparateLayerOutLine(){
//    int tempX1;
//    int tempY1;
//    int tempZ1;
//    int tempX2;
//    int tempY2;
//    int tempZ2;
//    point temp1;
//    point temp2;
//    int index1;
//    int index2;
//    if (pointSeparation.size()>0){
//        index1=0;
//        temp1.X=pointSeparation[index1].X;
//        temp1.Y=pointSeparation[index1].Y;
//        temp1.Z=pointSeparation[index1].Z;
//        OutLineSeparation.push_back(temp1);
//        while (pointSeparation.size()>0){
//        index1++;
//        for (int i=0;i<pointSeparation.size();i++){
//            if (temp1.X==pointSeparation[index1].X &&
//                temp1.Y==pointSeparation[index1].Y &&
//                temp1.Z==pointSeparation[index1].Z){
//                    temp2.X=pointSeparation[index1+1].X;
//                    temp2.Y=pointSeparation[index1+1].Y;
//                    temp2.Z=pointSeparation[index1+1].Z;
//                    OutLineSeparation.push_back(temp2);
//                    pointSeparation.erase(pointSeparation.begin()+index1,pointSeparation.begin()+index1+1);
//                    cout<<index1<<endl;
//                    system("pause");
//                    temp1.X=temp2.X;
//                    temp1.Y=temp2.Y;
//                    temp1.Z=temp2.Z;
//                    index1=0;
//                    break;
//            }
//        }
//        }
//    }
//}

///Вторая основная фукнция (запускается перед той что выше). Выполняет генерация точек пересечения плоскости сечения с треуголниками модели
void FindSeparatePoint(){
    int vert2;
    pointSeparation.clear();
    pointSeparationID.clear();
    ///Основной цикл прохода по всем треугольниками модели
    for(int trianN=0;trianN<triangleBase.size();trianN++){
        ///Преобразование с определением к какой грани относится та или иная точка на трегуольнике
        for (int vert1=0;vert1<3;vert1++){
            if (vert1==0) vert2=1;
            if (vert1==1) vert2=2;
            if (vert1==2) vert2=0;
            ///Оплетка для проверки факта парарельности треугольника секущей плоскости
            if (triangleBase[trianN].p[vert1].Z==SlicerHeight) {
                cout<<"Danger triangle pararell slisser plane "<<trianN<<endl;
                pointSeparation.clear();
                pointSeparationID.clear();
                return;
            }

            ///Определение факта того что плоскость пересекает треугольник (она должна пересекать точно 2 грани, не может быть 1 или 3)
            if ((triangleBase[trianN].p[vert1].Z<SlicerHeight && triangleBase[trianN].p[vert2].Z>SlicerHeight)||
                (triangleBase[trianN].p[vert1].Z>SlicerHeight && triangleBase[trianN].p[vert2].Z<SlicerHeight)){
                    float difX=triangleBase[trianN].p[vert2].X-triangleBase[trianN].p[vert1].X;
                    float difY=triangleBase[trianN].p[vert2].Y-triangleBase[trianN].p[vert1].Y;
                    float difZ=triangleBase[trianN].p[vert2].Z-triangleBase[trianN].p[vert1].Z;
                    float t=(SlicerHeight-triangleBase[trianN].p[vert1].Z)/(difZ);
//                    if (t<0.0001) {
//                        cout<<"DangerousLextrangeras! "<<vert1<<"-"<<vert2<<" trianN="<<trianN<<endl;
//                        cout<<"Z2="<<triangleBase[trianN].p[vert2].Z<<" Z1="<<triangleBase[trianN].p[vert1].Z<<" t="<<t<<endl;
//                        cout<<"difX="<<difX<<" difY="<<difY<<" difZ="<<difZ<<endl<<endl;
//                    }
                    if (pointSeparationID.size()>0){
                        if (pointSeparationID[pointSeparationID.size()-1]==trianN &&
                            (fabs(pointSeparation[pointSeparation.size()-1].X-(triangleBase[trianN].p[vert1].X+difX*t)))<0.001 &&
                            (fabs(pointSeparation[pointSeparation.size()-1].Y-(triangleBase[trianN].p[vert1].Y+difY*t)))<0.001){
                                //cout<<pointSeparation[pointSeparation.size()-1].X<<" "<<triangleBase[trianN].p[vert1].X+(difX)*t<<endl;
                                pointSeparation.pop_back();
                                pointSeparationID.pop_back();
                                //cout<<"in triangle# "<<trianN<<" find zeroDiff"<<endl<<endl;
                                continue;
                        }
                        //cout.setf(ios::fixed);
                        //cout.precision(50);
                        //else cout<<"bug "<<pointSeparation[pointSeparation.size()-1].X-(triangleBase[trianN].p[vert1].X+(difX)*t)<<endl<<endl;
                    }
                    point temp;
                    temp.X=triangleBase[trianN].p[vert1].X+difX*t;
                    temp.Y=triangleBase[trianN].p[vert1].Y+difY*t;
                    temp.Z=triangleBase[trianN].p[vert1].Z+difZ*t;
                    pointSeparation.push_back(temp);
                    pointSeparationID.push_back(trianN);
//                    cout<<"vert#"<<vert1<<"-"<<vert2<<" ID="<<trianN<<endl;
//                    cout<<"Zpos "<<triangleBase[trianN].p[vert2].Z<<"-"<<triangleBase[trianN].p[vert1].Z<<" t="<<t<<endl;
//                    cout<<"difX="<<difX<<" difY="<<difY<<" difZ="<<difZ<<endl;
//                    cout<<"newX="<<temp.X<<" newY"<<temp.Y<<" newZ"<<temp.Z<<" ID="<<trianN<<endl<<endl;
            }
        }
    }

//        if ((fabs(OutLineSeparation[i].X-OutLineSeparation[i+1].X)+fabs(OutLineSeparation[i].Y-OutLineSeparation[i+1].Y))<0.0001){

}

///Функция поиска габаритов модели
void FindGabarite(){
    float minX=triangleBase[0].p[0].X;
    float maxX=triangleBase[0].p[0].X;
    float minY=triangleBase[0].p[0].Y;
    float maxY=triangleBase[0].p[0].Y;
    float minZ=triangleBase[0].p[0].Z;
    float maxZ=triangleBase[0].p[0].Z;
    for(int i=0;i<triangleBase.size();i++){
        for (int j=0;j<3;j++){
            if (triangleBase[i].p[j].X>maxX) maxX=triangleBase[i].p[j].X;
            if (triangleBase[i].p[j].Y>maxY) maxY=triangleBase[i].p[j].Y;
            if (triangleBase[i].p[j].Z>maxZ) maxZ=triangleBase[i].p[j].Z;
            if (triangleBase[i].p[j].X<minX) minX=triangleBase[i].p[j].X;
            if (triangleBase[i].p[j].Y<minY) minY=triangleBase[i].p[j].Y;
            if (triangleBase[i].p[j].Z<minZ) minZ=triangleBase[i].p[j].Z;
        }
    }
    GabariteMaxX=maxX;
    GabariteMinX=minX;
    GabariteMaxY=maxY;
    GabariteMinY=minY;
    GabariteMaxZ=maxZ;
    GabariteMinZ=minZ;
}

///Сдвижка объекта таким образом чтобы его нижнаяя грань касалась стола печати
void PlateBaseBody(){
    float offsetZ=0;
    FindGabarite();
    if (GabariteMinZ<0) offsetZ=-GabariteMinZ;
    if (GabariteMinZ>0) offsetZ=GabariteMinZ;
    for (int i=0;i<triangleBase.size();i++){
        for (int j=0;j<3;j++){
            triangleBase[i].p[j].Z=triangleBase[i].p[j].Z+offsetZ;
        }
    }
    FindGabarite();
}

///Функция повортота всей модели вокруг своего центра
void RotateBody(int axis){
    FindGabarite();
    float NewNormalX, NewNormalY, NewNormalZ;
    float NewPointX, NewPointY, NewPointZ;
    float PointMidleX, PointMidleY, PointMidleZ;
    ///Определение центра модели
    PointMidleX=((GabariteMaxX-GabariteMinX)/2)+GabariteMinX;
    PointMidleY=((GabariteMaxY-GabariteMinY)/2)+GabariteMinY;
    PointMidleZ=((GabariteMaxZ-GabariteMinZ)/2)+GabariteMinZ;
    ///Цикл сдвижки кадой точки через матрицу поворота в 3D пространстве
    for (int i=0;i<triangleBase.size();i++){
        if (axis==0){
            NewNormalX=triangleBase[i].Normal.X;
            NewNormalY=triangleBase[i].Normal.Y*cos(M_PI/4)+triangleBase[i].Normal.Z*sin(M_PI/4);
            NewNormalZ=-triangleBase[i].Normal.Y*sin(M_PI/4)+triangleBase[i].Normal.Z*cos(M_PI/4);
        }
        if (axis==1){
            NewNormalX=triangleBase[i].Normal.X*cos(M_PI/4)+triangleBase[i].Normal.Z*sin(M_PI/4);
            NewNormalY=triangleBase[i].Normal.Y;
            NewNormalZ=-triangleBase[i].Normal.X*sin(M_PI/4)+triangleBase[i].Normal.Z*cos(M_PI/4);
        }
        if (axis==2){
            NewNormalX=triangleBase[i].Normal.X*cos(M_PI/4)-triangleBase[i].Normal.Y*sin(M_PI/4);
            NewNormalY=triangleBase[i].Normal.X*sin(M_PI/4)+triangleBase[i].Normal.Y*cos(M_PI/4);
            NewNormalZ=triangleBase[i].Normal.Z;
        }
        ///Дополнительное смещение нормалей модели для верного вычисления освещения модели
        triangleBase[i].Normal.X=NewNormalX;
        triangleBase[i].Normal.Y=NewNormalY;
        triangleBase[i].Normal.Z=NewNormalZ;
        for (int j=0;j<3;j++){
            if (axis==0){
                NewPointX=triangleBase[i].p[j].X;
                NewPointY=((triangleBase[i].p[j].Y-PointMidleY)*cos(M_PI/4)+(triangleBase[i].p[j].Z-PointMidleZ)*sin(M_PI/4));
                NewPointZ=(-1*(triangleBase[i].p[j].Y-PointMidleY)*sin(M_PI/4)+(triangleBase[i].p[j].Z-PointMidleZ)*cos(M_PI/4));
            }
            if (axis==1){
                NewPointX=((triangleBase[i].p[j].X-PointMidleX)*cos(M_PI/4)+(triangleBase[i].p[j].Z-PointMidleZ)*sin(M_PI/4));
                NewPointY=triangleBase[i].p[j].Y;
                NewPointZ=(-1*(triangleBase[i].p[j].X-PointMidleY)*sin(M_PI/4)+(triangleBase[i].p[j].Z-PointMidleZ)*cos(M_PI/4));
            }
            if (axis==2){
                NewPointX=((triangleBase[i].p[j].X-PointMidleX)*cos(M_PI/4)-(triangleBase[i].p[j].Y-PointMidleY)*sin(M_PI/4));
                NewPointY=((triangleBase[i].p[j].X-PointMidleY)*sin(M_PI/4)+(triangleBase[i].p[j].Y-PointMidleY)*cos(M_PI/4));
                NewPointZ=triangleBase[i].p[j].Z;
            }
            triangleBase[i].p[j].X=NewPointX;
            triangleBase[i].p[j].Y=NewPointY;
            triangleBase[i].p[j].Z=NewPointZ;
        }
    }
    FindGabarite();
}

///Функция чтения файла STL и преобразование его в формат программы. По сути функция - это синтаксический разбор фалйа
bool ReadSTLFile(string a){
    triangleBase.clear();
    string s;
    ifstream myfile (a.c_str());

    if (myfile.is_open())
    {
        int State=0;
        int Type=0;
        int countCoords=0;
        float points[3];
        triangle form;
        ///В случае верного открытия файла производится полное чтение всех строк кода, касающиеся одной SOLID структуры
        while ( myfile.good() )
        {
            getline (myfile,s,' ');
            if (s!=""){
                if (State==1) {
                    countCoords--;
                    points[countCoords]=atof(s.c_str());
                    ///Из файла последовательно выбираются координаты точек треугольника и кооринаты нормалей
                    if (countCoords==0) {
                       if (Type==1) {
                            form.Normal.X=points[2];
                            form.Normal.Y=points[1];
                            form.Normal.Z=points[0];
                            triangleBase.push_back(form);
                       }
                       if (Type>1) {
                            triangleBase[triangleBase.size()-1].p[Type-2].X=points[2];
                            triangleBase[triangleBase.size()-1].p[Type-2].Y=points[1];
                            triangleBase[triangleBase.size()-1].p[Type-2].Z=points[0];
                       }
                       State=0;
                    }
                }
                ///Небольшая машина состояний для синтаксического разбора файла
                if (State==0) {
                    if (s=="normal"){
                        State=1;
                        countCoords=3;
                        Type=1;
                    }
                    if (s=="vertex"){
                        State=1;
                        countCoords=3;
                        Type=Type+1;
                    }
                }
                //cout << atof(s.c_str()) << endl;
            }
        }
        myfile.close();
    }
    FindGabarite();

    ///После чтения выводится количество прочитанных треугольников
    cout<<"Triangle in file = "<<triangleBase.size()<<endl;

    if (triangleBase.size()>0) return true;
    return false;
}

///Системная функция изменение размера отображающего окна
static void resize(int width, int height)
{
    const float ar = (float) width / (float) height;

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //glFrustum(-ar, ar, -1.0, 1.0, 2.0, 100.0);

    gluPerspective(45, ar, 1.0f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity() ;
}

///Основная функция вывода на экран в формате GLUT
static void display(void)
{
    const double t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
    const double a = t*90.0;

    ///Подготовка сцены
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    //gluLookAt(0, 0, 6,     0, 0, 0,     0, 0, 1);
    glTranslatef(0,0,-5);
    glPushMatrix();

    ///Поворот сцены
    glRotatef(angleCamRotateY+DeltaAngleCamRotateY,1,0,0);
    glRotatef(angleCamRotateX+DeltaAngleCamRotateX,0,1,0);

    glScalef(zoomScene,zoomScene,zoomScene);

    //glColor4f(0.2,0.2,0.8,0.5);
    //glutSolidTeapot(1.0);

    ///Отображение осей координат
    glLineWidth(4);
    glBegin(GL_LINES);
        glNormal3f(0,0,1);
        glColor4f(1,0,0,1);
        glVertex3f(-0.3,0,0);
        glVertex3f(1,0,0);
        glColor4f(0,1,0,1);
        glVertex3f(0,-0.3,0);
        glVertex3f(0,1,0);
        glColor4f(0,0,1,1);
        glVertex3f(0,0,-0.3);
        glVertex3f(0,0,1);
    glEnd();

    ///Отображение полигонов в виде замкнутых многоугольников ( обвчно пустой)
    glLineWidth(1);
    glBegin(GL_LINES);
        glColor4f(1,1,1,0.5);
        glNormal3f(0.5,0.5,0.5);
        for (int i=0;i<PoligoneBase.size();i++){
            for (int j=0;j<PoligoneBase[i].StackCorners.size()-1;j++){
                glVertex3f(PoligoneBase[i].StackCorners[j].x,PoligoneBase[i].StackCorners[j].y,SlicerHeight);
                glVertex3f(PoligoneBase[i].StackCorners[j+1].x,PoligoneBase[i].StackCorners[j+1].y,SlicerHeight);
            }
            glVertex3f(PoligoneBase[i].StackCorners[PoligoneBase[i].StackCorners.size()-1].x,PoligoneBase[i].StackCorners[PoligoneBase[i].StackCorners.size()-1].y,SlicerHeight);
            glVertex3f(PoligoneBase[i].StackCorners[0].x,PoligoneBase[i].StackCorners[0].y,SlicerHeight);
        }
    glEnd();

    ///Отобржние замнутого внешнего контура модели после слайсинга (красный)
    glLineWidth(3);
    glBegin(GL_LINES);
        glColor4f(1,0,0,1);
        glNormal3f(0,0,1);
        for (int i=0;i<offsetUpForLinePerimetr-1;i++){
            if (OutLineSeparationID[i+1]==OutLineSeparationID[i]){
                glVertex3f(OutLineSeparation[i].X,OutLineSeparation[i].Y,OutLineSeparation[i].Z);
                glVertex3f(OutLineSeparation[i+1].X,OutLineSeparation[i+1].Y,OutLineSeparation[i+1].Z);
            }
        }
    glEnd();

//    ///
//    glLineWidth(3);
//    glBegin(GL_LINES);
//        glColor4f(1,0,0,1);
//        glNormal3f(0,0,1);
//        for (int i=0;i<offsetUpForLinePerimetr-1;i++){
//            if (OutLineSeparationID[i+1]==OutLineSeparationID[i]){
//                glVertex3f(OutLineSeparation[i].X,OutLineSeparation[i].Y,OutLineSeparation[i].Z);
//                glVertex3f(OutLineSeparation[i+1].X,OutLineSeparation[i+1].Y,OutLineSeparation[i+1].Z);
//            }
//        }
//    glEnd();

    ///Отображение множетсва слоев после слайсинга модели (зеленый)
    glLineWidth(2);
    for (int j=0;j<OutLineLoop.size();j++){
        glBegin(GL_LINES);
        glColor4f(0,1,0,1);
        glNormal3f(0,0,1);
        for (int i=0;i<OutLineLoop.at(j).size()-1;i++){
            if (OutLineLoopID.at(j).at(i+1)==OutLineLoopID.at(j).at(i)){
                glVertex3f(OutLineLoop.at(j).at(i).X,OutLineLoop.at(j).at(i).Y,OutLineLoop.at(j).at(i).Z);
                glVertex3f(OutLineLoop.at(j).at(i+1).X,OutLineLoop.at(j).at(i+1).Y,OutLineLoop.at(j).at(i+1).Z);
            }
        }
        glEnd();
    }

    ///Отображение точек в процессе подготовки к заливке сеткой вороного
    glPointSize(4);
    glBegin(GL_POINTS);
        glColor4f(0,0,1,1);
        for (int i=0;i<InnerPoints.size();i++){
            glVertex3f(InnerPoints[i].X,InnerPoints[i].Y,InnerPoints[i].Z);
        }
    glEnd();

    ///Переопределение первой точки вывода заливки
    glPointSize(8);
    glBegin(GL_POINTS);
        glColor4f(0,0,1,1);
        if (InnerPoints.size()>0) glVertex3f(InnerPoints[0].X,InnerPoints[0].Y,InnerPoints[0].Z);
    glEnd();

    ///Отображение точек после процесса подогтовки к разбиению вороного ( виснет)
    glPointSize(6);
    glBegin(GL_POINTS);
        glColor4f(1,1,1,1);
        for (int i=0;i<MeabyPoint.size();i++){
            glVertex3f(MeabyPoint[i].x,MeabyPoint[i].y,SlicerHeight);
        }
    glEnd();

//    glBegin(GL_LINES);
//        for (int i=-10;i<=10;i++){
//            glLineWidth(2);
//            glColor3f(0.5,0.5,0.5);
//            glVertex3f(-10,i,0);
//            glVertex3f(10,i,0);
//            glVertex3f(i,-10,0);
//            glVertex3f(i,10,0);
//            glLineWidth(1);
//            glColor3f(0.3,0.3,0.3);
//            glVertex3f(-10,i+0.5,0);
//            glVertex3f(10,i+0.5,0);
//            glVertex3f(i+0.5,-10,0);
//            glVertex3f(i+0.5,10,0);
//        }
//    glEnd();

    ///Отработка момента скрытия модели и отображения модели
    if (HideSolid) {
        glBegin(GL_TRIANGLES);
            glColor4f(0.8,0.8,0.1,0.8);
            for (int i=0;i<triangleBase.size();i++){
                glNormal3f(triangleBase[i].Normal.X,triangleBase[i].Normal.Y,triangleBase[i].Normal.Z);
                glVertex3f(triangleBase[i].p[0].X,triangleBase[i].p[0].Y,triangleBase[i].p[0].Z);
                glVertex3f(triangleBase[i].p[1].X,triangleBase[i].p[1].Y,triangleBase[i].p[1].Z);
                glVertex3f(triangleBase[i].p[2].X,triangleBase[i].p[2].Y,triangleBase[i].p[2].Z);
            }
        glEnd();
    }

    ///Отображение плоскости слайсинга
    glColor4f(0.1,0.1,0.5,0.3);
    glBegin(GL_POLYGON);
        glNormal3f(0,0,1);
        glVertex3f(GabariteMinX-1,GabariteMinY-1,SlicerHeight);
        glVertex3f(GabariteMaxX+1,GabariteMinY-1,SlicerHeight);
        glVertex3f(GabariteMaxX+1,GabariteMaxY+1,SlicerHeight);
        glVertex3f(GabariteMinX-1,GabariteMaxY+1,SlicerHeight);
    glEnd();

    ///Отображение точек пересечения модели и плоскости слайсинга
    glPointSize(4);
    glBegin(GL_POINTS);
        glColor4f(1,0,0,1);
        for (int i=0;i<pointSeparation.size();i++){
            glVertex3f(pointSeparation[i].X,pointSeparation[i].Y,pointSeparation[i].Z);
        }
    glEnd();

    glPopMatrix();

    glutSwapBuffers();
}

///функция определяюща расстояние от точки до прямой по наименьшему пути - перпенликоуляру
float OffsetByLine(point P1, point P2, point O){
    float h=((P2.X-P1.X)*(O.Y-P1.Y)-(P2.Y-P1.Y)*(O.X-P1.X))/(float)sqrt((float)pow((P2.X-P1.X)+(P2.Y-P1.Y),2));
    return h<0?-h:h;
}

///Функция генерирующая множество контуров - полный слайсиг модели
static void GenerateLoopCycle(void){
    ///Подготовка к слайсингу и определение параметров слайсинга
    if (OutLineSeparation.size()>0){
        cout<<"Count vertex = "<<OutLineSeparation.size()<<endl;
    }
    ///Отчистка слоев и запоминание текущей высоты слайсинга
    float tempSliceHight = SlicerHeight;
    vector <point> tempLoop;
    vector <int> tempLoopID;

    OutLineLoop.clear();
    OutLineLoopID.clear();

    ///Цикл слайсинга с использованием базовой фунции слайсинга слоя
    for (float height=LayerHeight/2;height<GabariteMaxZ;height+=LayerHeight){
        SlicerHeight=height;
        FindSeparatePoint();
        FindSeparateLayerOutLine();
        tempLoop=OutLineSeparation;
        tempLoopID=OutLineSeparationID;
        OutLineLoop.push_back(tempLoop);
        OutLineLoopID.push_back(tempLoopID);
    }

    OutLineSeparation.clear();
    OutLineSeparationID.clear();

    SlicerHeight=tempSliceHight;
}

///Функция генерирующая множество контуров - полный слайсиг модели с функцией адаптивного подхода к высоте слоя
static void GenerateLoopCycleAdaptive(void){
    ///Подготовка к слайсингу и определение параметров слайсинга
    if (OutLineSeparation.size()>0){
        cout<<"Count vertex = "<<OutLineSeparation.size()<<endl;
    }
    ///Создание временных подслоев для определения их степени похожести
    float tempSliceHight = SlicerHeight;
    vector <point> tempLoop;
    vector <int> tempLoopID;

    vector <point> tempLoop2;
    vector <int> tempLoopID2;

    OutLineLoop.clear();
    OutLineLoopID.clear();

    ///Цикл слайсинга с использованием базовой фунции слайсинга слоя
    for (float height=LayerHeight/2;height<GabariteMaxZ-LayerHeight;height+=LayerHeight){
        ///Создание подслоев для определения их степени похожести. ПОдслайсинг слоев
        SlicerHeight=height;
        FindSeparatePoint();
        FindSeparateLayerOutLine();
        tempLoop=OutLineSeparation;
        tempLoopID=OutLineSeparationID;

        SlicerHeight=height+LayerHeight;
        FindSeparatePoint();
        FindSeparateLayerOutLine();
        tempLoop2=OutLineSeparation;
        tempLoopID2=OutLineSeparationID;

        ///Цикл поиска степени похожести
        float minDeltaOnLoop=300;
        for (int i=0;i<tempLoop2.size();i++){
            float minDelta=300;
            for (int j=0;j<tempLoop.size()-1;j++){
                if (tempLoopID[j+1]==tempLoopID[j]){
                    if (OffsetByLine(tempLoop[j],tempLoop[j+1],tempLoop2[i])<minDelta)
                        ///На кадом этапе определяется переменная - MinDelta - по сути после поиска по всем вершинам - это макисмальное из минимальных для каждой точки значение отклонения точек от исходных линий
                        minDelta=OffsetByLine(tempLoop[j],tempLoop[j+1],tempLoop2[i]);
                }
            }
            if (minDelta<minDeltaOnLoop) minDeltaOnLoop=minDelta;
        }

        cout<<"Adaptive = "<<minDeltaOnLoop<<endl;
        //system("pause");
        ///Собственно тут принимается решение когда делать слой одинаковым. Если величина отклооения меньше 0.001 мм. То делать слой в 2 раза больше
        if (minDeltaOnLoop<=0.00001) {
            height+=LayerHeight;
            OutLineLoop.push_back(tempLoop2);
            OutLineLoopID.push_back(tempLoopID2);
            //supsees=true;
        }
        else{
            OutLineLoop.push_back(tempLoop);
            OutLineLoopID.push_back(tempLoopID);
        }
    }

    OutLineSeparation.clear();
    OutLineSeparationID.clear();

    SlicerHeight=tempSliceHight;
}

float LenghtOfLine(point a, point b){
    return sqrt(pow((a.X-b.X),2)+pow((a.Y-b.Y),2));
}

void exportGcode(){
    ofstream fout ("Export.gcode");
    fout << "M140 S50.000000"<<endl
<<"M109 T0 S190.000000"<<endl
<<"T0"<<endl
<<"M190 S50.000000"<<endl
<<"G21        ;metric values"<<endl
<<"G90        ;absolute positioning"<<endl
<<"M83        ;set extruder to absolute mode"<<endl
<<"M107       ;start with the fan off"<<endl
<<"G28 X0 Y0  ;move X/Y to min endstops"<<endl
<<"G28 Z0     ;move Z to min endstops"<<endl
<<"G1 Z15.0 F9000 ;move the platform down 15mm"<<endl
<<"G92 E0                  ;zero the extruded length"<<endl
<<"G1 F200 E3              ;extrude 3mm of feed stock"<<endl
<<"G92 E0                  ;zero the extruded length again"<<endl
<<"G1 F9000"<<endl
<<"M107"<<endl;

float centX=92.22;
float centY=86.75;

    for (int j=0;j<OutLineLoop.size();j++){
        fout<<";LAYER:"<<j<<endl;
        fout<<"G0 F9000 X"<<OutLineLoop.at(j).at(0).X+centX<<" Y"<<OutLineLoop.at(j).at(0).Y+centY<<" Z"<<OutLineLoop.at(j).at(0).Z<<endl;
        fout<<";TYPE:WALL-OUTER"<<endl;
        //fout<<"G0 F1200 X"<<OutLineLoop.at(j).at(0).X+centX<<" Y"<<OutLineLoop.at(j).at(0).Y+centY<<" E"<<LenghtOfLine()<<endl;
        for (int i=1;i<OutLineLoop.at(j).size()-1;i++){
            if (OutLineLoopID.at(j).at(i-1)==OutLineLoopID.at(j).at(i)){
                fout<<"G1 F1200 X"<<OutLineLoop.at(j).at(i).X+centX<<" Y"<<OutLineLoop.at(j).at(i).Y+centY<<" E"<<(LenghtOfLine(OutLineLoop.at(j).at(i),OutLineLoop.at(j).at(i-1))/63.697)<<endl;
            }
        }
        fout<<"G1 F1200 X"<<OutLineLoop.at(j).at(OutLineLoop.at(j).size()-1).X+centX<<" Y"<<OutLineLoop.at(j).at(OutLineLoop.at(j).size()-1).Y+centY<<" E0.1"<<endl;
    }

    fout.close ();
    cout<<"Save to file is OK!"<<endl;
}

///Большая фунция отлова нажатия клавиш, большой Swithc который вызывает функции описанные выше
static void key(unsigned char key, int x, int y)
{
    string NewFile;
    WIN32_FIND_DATA FindFileData;
    HANDLE hf;
    //cout<<(int)key<<endl;
    switch (key)
    {
        case 27 :
        case 'q':
            exit(0);
            break;

        case 'p':
        case 231:
            PlateBaseBody();
            break;

        case 'w':
        case 246:
            glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
            break;

        case 'W':
        case 214:
            glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
            break;

        case 'a':
        case 244:
            FindSeparatePoint();
            FindSeparateLayerOutLine();
            break;

        case 's':
        case 251:
            FindSeparatePoint();
            //cout<<"Points-"<<pointSeparation.size()<<endl;
            break;

        case 'd':
        case 226:
            FindSeparateLayerOutLine();
            //cout<<"LinePoints-"<<OutLineSeparation.size()<<endl;
            break;

        case 'o':
        case 249:
            cout<<"File in roots dir:"<<endl;

            hf=FindFirstFile("*.stl",&FindFileData);
            if (hf!=INVALID_HANDLE_VALUE){
                do{
                    cout<<FindFileData.cFileName<<endl;
                }
                while (FindNextFile(hf,&FindFileData)!=0);
                FindClose(hf);
            }
            cout<<"Input File Name"<<endl;
            cin>>NewFile;
            ReadSTLFile(NewFile);
            break;

        case 'F':
        case 192:
            InnerPoints.clear();
            SetInnerPointsGrid();
            break;

        case 'f':
        case 224:
            InnerPoints.clear();
            SetInnerPointsRand();
            point tmp;
            break;

        case 'g':
        case 239:
            //CreatePligoneForPoint(0);
            break;

        case 'x':
        case 247:
            HideSolid=false;
            break;

        case 'X':
        case 215:
            HideSolid=true;
            break;

        case 'h':
        case 240:
            cout<<"s - slice model and generate separate points"<<endl;
            cout<<"d - connect separate points in loops"<<endl;
            cout<<"f - generate many points into loops"<<endl;
            cout<<"e - rotate stl model by axis X"<<endl;
            cout<<"r - rotate stl model by axis Y"<<endl;
            cout<<"t - rotate stl model by axis Z"<<endl;
            cout<<"x - hide solid model"<<endl;
            cout<<"File in roots dir:"<<endl;


            hf=FindFirstFile("*.stl",&FindFileData);
            if (hf!=INVALID_HANDLE_VALUE){
                do{
                    cout<<FindFileData.cFileName<<endl;
                }
                while (FindNextFile(hf,&FindFileData)!=0);
                FindClose(hf);
            }
            //cout<<IntersectLine2Otrezok(-3,0,-2.5,-1,0,0,-0.5,-1)<<endl;
            //cout<<"points"<<endl;
//            for (int i=0;i<pointSeparation.size();i++){
//                //cout<<"P#"<<i<<" ID="<<pointSeparationID[i]<<" X="<<pointSeparation[i].X<<" Y="<<pointSeparation[i].Y<<" Z="<<pointSeparation[i].Z<<endl;
//            }
//            //cout<<endl<<"lines"<<endl;
//            for (int i=0;i<OutLineSeparation.size()-1;i++){
//                //cout<<"Line#"<<i<<" delta="<<fabs(OutLineSeparation[i].X-OutLineSeparation[i+1].X)+fabs(OutLineSeparation[i].Y-OutLineSeparation[i+1].Y)+fabs(OutLineSeparation[i].Z-OutLineSeparation[i+1].Z)
//                    //<<" NumberLoop="<<OutLineSeparationID[i]<<endl;
//            }
            cout<<endl;
            break;

        case '[':
        case 245:
            //cout<<offsetUpForLinePerimetr<<endl;
            offsetUpForLinePerimetr=offsetUpForLinePerimetr-1;
            if (offsetUpForLinePerimetr<0) offsetUpForLinePerimetr=0;
            break;

        case ']':
        case 250:
            //cout<<offsetUpForLinePerimetr<<endl;
            offsetUpForLinePerimetr=offsetUpForLinePerimetr+1;
            if (offsetUpForLinePerimetr>OutLineSeparation.size()) offsetUpForLinePerimetr=OutLineSeparation.size();
            break;

        case 'e':
        case 243:
            RotateBody(0);
            break;

        case 'r':
        case 234:
            RotateBody(1);
            break;

        case 't':
        case 229:
            RotateBody(2);
            break;

        case '-':
            SlicerHeight-=LayerHeight;
            break;

        case 'n':
        case 242:
            GenerateLoopCycle();
            break;

        case 'm':
        case 252:
            GenerateLoopCycleAdaptive();
            break;

        case '=':
            SlicerHeight+=LayerHeight;
            break;

        case 'l':
        case 228:
            cout<<"Input Layer Height: ";
            cin>>LayerHeight;
            break;

        case 'v':
        case 236:
            exportGcode();
            break;
    }
    glutPostRedisplay();
}

///Фнуциия обработки движения и нажатия клавиш на мышь - нужна для вращения
void mouse(int button, int state, int x, int y)
{
    if (button==3) {
        zoomScene=zoomScene*1.2;
    }
    if (button==4) {
        zoomScene=zoomScene/1.2;
    }
    if((button==GLUT_LEFT_BUTTON)&&(state==GLUT_DOWN))
    {
         StartAngleCamRotateX=x;
         StartAngleCamRotateY=y;
    }
    if((button==GLUT_LEFT_BUTTON)&&(state==GLUT_UP))
    {
         angleCamRotateY=angleCamRotateY+DeltaAngleCamRotateY;
         angleCamRotateX=angleCamRotateX+DeltaAngleCamRotateX;
         DeltaAngleCamRotateX=0;
         DeltaAngleCamRotateY=0;
    }
    glutPostRedisplay();
}

void mouseMov(int x, int y){
    DeltaAngleCamRotateX=x-StartAngleCamRotateX;
    DeltaAngleCamRotateY=y-StartAngleCamRotateY;
}

///ПРостой программы
static void idle(void)
{
    glutPostRedisplay();
}

const GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
const GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

const GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

///Точка входа в программу - основная функция - с нее начинается программа
int main(int argc, char *argv[])
{
    ///Инициализация графического окна
    glutInit(&argc, argv);
    glutInitWindowSize(640,480);
    glutInitWindowPosition(1300-640,10);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

    glutCreateWindow("Slicer TurboMaxHiTechProSpeedUpUltraHighMegaUltra");

    ///Определение glutовских функций
    glutReshapeFunc(resize);
    glutDisplayFunc(display);
    glutKeyboardFunc(key);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMov);
    glutIdleFunc(idle);

    ///Предочистка экрана
    glClearColor(1,1,1,1);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    ///Настройки глута - нужны для верного отображения
    glEnable(GL_DEPTH_TEST);
//    glDepthFunc(GL_ALWAYS);
//    glDepthMask(GL_TRUE);
//    glClearDepth(1.0f);
    glFrontFace(GL_CW);

    ///Настройка света и прозрачности
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFunc(GL_ONE_MINUS_DST_ALPHA,GL_DST_ALPHA);
    //glEnable(GL_POLYGON_STIPPLE);
    //glBlendFunc(GL_ONE,GL_ONE);

    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_LIGHTING);

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
//    glShadeModel(GL_FLAT);
//    glEnable(GL_SMOOTH);

    glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glMaterialfv(GL_FRONT, GL_AMBIENT,   mat_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE,   mat_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR,  mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);

    ///Начальные настройки программы при старте
    angleCamRotateX=-45;
    angleCamRotateY=45;
    zoomScene=1;
    SlicerHeight=0;
    countLoops=0;
    HideSolid=true;
    cout.precision(6);
    LayerHeight=0.2;

    ///ЧТобы при старте что то было уже открыто
    //string a="C:\\DetN15.stl";
    //ReadSTLFile(a);

    cout<<"Program Slicer ProTURBO v2.15"<<endl;
    cout<<"s - slice model and generate separate points"<<endl;
    cout<<"d - connect separate points in loops"<<endl;
    cout<<"f - generate many points into loops"<<endl;
    cout<<"e - rotate stl model by axis X"<<endl;
    cout<<"r - rotate stl model by axis Y"<<endl;
    cout<<"t - rotate stl model by axis Z"<<endl;
    cout<<"x - hide solid model"<<endl;
    cout<<"Shift+x - show solid model"<<endl;
    cout<<"w - show wireframe model"<<endl;
    cout<<"Shift+w - hide wireframe model"<<endl;
    cout<<"o - open file"<<endl;
    cout<<"a - auto generate one layer"<<endl;
    cout<<"n - generate loop sequance"<<endl;
    cout<<"m - generate loop sequance adaptive"<<endl;
    cout<<"l - input height layer"<<endl;
    cout<<"v - save to file"<<endl;

    glutMainLoop();

    return EXIT_SUCCESS;
}



//----------------------------------------------
//            float a=sqrt(pow(p1x,2)+pow(p1y,2));
//            float b=sqrt(pow(p2x,2)+pow(p2y,2));
//            float c=sqrt(pow(p2x-p1x,2)+pow(p2y-p1y,2));
//            float cosA=(pow(a,2)+pow(b,2)-pow(c,2))/(2*a*b);
//            cout<<acos(cosA)*180/M_PI<<endl;
//            if (i==0){
//                psevdoX=p1x;
//                psevdoY=p1y;
//            }
//            float aTo1=sqrt(pow(psevdoX,2)+pow(psevdoY,2));
//            float bTo1=sqrt(pow(p1x,2)+pow(p1y,2));
//            float cTo1=sqrt(pow(psevdoX-p1x,2)+pow(psevdoY-p1y,2));
//            float cosATo1=(pow(aTo1,2)+pow(bTo1,2)-pow(cTo1,2))/(2*aTo1*bTo1);
//            cout<<acos(cosATo1)*180/M_PI<<endl;
//            float aTo2=sqrt(pow(psevdoX,2)+pow(psevdoY,2));
//            float bTo2=sqrt(pow(p2x,2)+pow(p2y,2));
//            float cTo2=sqrt(pow(psevdoX-p2x,2)+pow(psevdoY-p2y,2));
//            float cosATo2=(pow(aTo2,2)+pow(bTo2,2)-pow(cTo2,2))/(2*aTo2*bTo2);
//            cout<<acos(cosATo2)*180/M_PI<<endl;
//
//            psevdoX=p1x;
//            psevdoY=p1y;
//
//            if (acos(cosATo1)*180/M_PI>=acos(cosATo2)*180/M_PI) {
//                dAlpha+=acos(cosA)*180/M_PI;
//            }
//            else {
//                dAlpha-=acos(cosA)*180/M_PI;
//            }
//            cout<<"AngleP1="<<acos(cosATo1)*180/M_PI<<" AngleP2="<<acos(cosATo2)*180/M_PI<<endl;
//            if ((dAlpha>360-0.001&&dAlpha<360+0.001)||(dAlpha>-360-0.001&&dAlpha<-360+0.001)){
//
//            }
//            cout<<"POINT#"<<InnerPoints.size()<<" SEICHAS Delta ALPHA="<<dAlpha<<" For point on line#"<<i<<endl<<endl;
//--------------------------------------------------------
