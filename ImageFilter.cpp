//
// Created by aless on 15/03/2023.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

using namespace std;


class edgeGradient{
    /*Class containing the following info about the edges:
     * gradientX_ contains the horizontal edge gradient
     * gradientY_ contains the vertical edge gradient
     * magnitude_ contains the magnitude of the edge vector [0, 255]
     * angle_ contains the direction of the edge vector [0, 2pi)
     */

private:
    int gradientX_;
    int gradientY_;
    double magnitude_;
    double angle_;



public:
    edgeGradient(){}
    edgeGradient(int gradientX, int gradientY ){
        gradientX_ = gradientX;
        gradientY_ = gradientY;
        ComputeAngleMagnitude();
    }


    void setGradientX(int gradientX) {
        gradientX_ = gradientX;
    }
    void setGradientY(int gradientY) {
        gradientY_ = gradientY;
    }


    int getGradientY() const {
        return gradientY_;
    }
    int getGradientX() const {
        return gradientX_;
    }

    int getMagnitude() const {
        return magnitude_;
    }

    double getAngle() const {
        return angle_;
    }

    void ComputeAngleMagnitude(){
        /*method to compute the angle and the magnitude of edge vector starting from the gradientX_ and gradientY_*/

        double magnitude = sqrt(gradientX_*gradientX_ + gradientY_*gradientY_);
        double x = gradientX_/magnitude;
        double y = gradientY_/magnitude;

        if(magnitude == 0.0) {
            angle_ = 0.0;
        }else {
            angle_ = atan2(y, x);
            if(angle_ < 0.0){
                /* All of the negative angles are translated of 2pi in order to have values in [0,2pi) */
                angle_ = angle_ + 2* M_PI;
            }
        }
        //normalization of the magnitude by tests shows much better results than clipping
        magnitude_ = (magnitude/1275.0)*255.0;
    }
};

struct weights{
    int red;
    int green;
    int blue;
};

//function to get back at the beginning of the line
void ToBeginLine(ifstream& file);

//function to avoid any comment line
string AvoidComments(ifstream& file);

//function to check if the cursor is at the beginning of the file
void CheckBeginningFile(ifstream file);

//function to save a picture on a file
void SaveOnFile(vector<vector<int>>& img, int width, int height, char name[], string header);

//Function to filter the image
edgeGradient EdgeFilter(vector<vector<int>>& img, int width, int height, int w, int h);

//Function to make the convolution of the kernel for a single pixel
edgeGradient EdgeFilterCalculation(vector<vector<int>>& img);

//Function to compute the color weights of one single pixel
weights WeightsComputation(double angle, double magnitude);



int main(int argc, char *argv[]){

    if(argc < 3){
        cout << "Not enough parameters provided \n";
        return 1;
    }

    //open the file in the position indicated by the first argument
    ifstream input_file(argv[1], ios::binary);
    if(!input_file){
        cout << "Error in opening the file";
        return 1;
    }

    string line;
    char fileHeader, test_char;
    int width, height;

    //the first line without comments is read
    line = AvoidComments(input_file);
    fileHeader = line[1];
    cout << "file header P" << fileHeader << "\n";

    if(fileHeader != '3'){
        cout << "file in the wrong format";
        return 1;
    }

    /*To be able to also use files that don't comply perfectly to the header standards the first line length is checked
     * if it is equal to 2 it means that only the file type is contained in it*/
    if(line.length() == 2) { //only the file type is present in the first line
        cout << "file doesn't respect standard but is readable";
        do {
            getline(input_file, line);
            cout << line;
        } while (line[0] == '#');

        ToBeginLine(input_file);
        input_file >> width >> height;
        cout << "\nwidth: " << width << "\n height: "<< height;

    }else if(line.length() > 2){ //file type width and height are all in the first line
        cout << "file does respect standard";
        ToBeginLine(input_file);
        //skips characters until after the file type
        for(int i = 0; i< 2; i++ ){
            input_file.get();
        }

        input_file >> width >> height;
        if (input_file.get() != '\n'){
            input_file.seekg(-1, ios_base::cur);
        }
        cout << "\nwidth: " << width << "\nheight: "<< height;

    }else{
        cout << "file not following the standard format and is unreadable";
        return 1;
    }

    //avoids any line containing comments and then gets back at the beginning of the first line containing the pixel
    //information
    AvoidComments(input_file);
    ToBeginLine(input_file);

    //instantiates the matrix that will contain grayscale pixel information
    vector<vector<int>> img(width, vector<int>(height));
    int depth, red, green, blue;

    //read the bit depth of the image
    input_file >> depth;
    cout << "\nbit_depth: " << depth<< "\n";

    //instantiates the matrix that will then contain all the information about the edges
    vector<vector<edgeGradient>> edgeInfoImg(height, vector<edgeGradient>(width));

    //Test matrix that contain the edges information
    //vector<vector<int>> edgeImg(height, vector<int>(width));

    //Instate the matrix that will contain the pixel information of the edge colored image
    vector<vector<int>> colorEdgeImg(height, vector<int>(width*3));


    //Reads the picture and transforms in it black and white
    for(int h = 0; h < height; h++){
        for(int w = 0; w < width; w++){
            input_file >> red >> green >> blue;
            img[h][w] = 0.2126 * red + 0.7152 * green + 0.0722 * blue;
        }

    }

    // Computes the edge filter in every pixel of the image then uses the info to computed also the color weights
    for(int h = 0; h < height; h++){
        for(int w = 0; w < width; w++){
            edgeInfoImg[h][w] = EdgeFilter(img, width, height, w, h);
            //edgeImg[h][w] = edgeInfoImg[h][w].getMagnitude();

            //Computes the color weights for each pixel
            weights weightsInfo = WeightsComputation(edgeInfoImg[h][w].getAngle(), edgeInfoImg[h][w].getMagnitude());
            colorEdgeImg[h][w*3] = weightsInfo.red;
            colorEdgeImg[h][w*3+1] = weightsInfo.green;
            colorEdgeImg[h][w*3+2] = weightsInfo.blue;
        }
    }


    //creates the header and than calls the function to save on the filename provided by console
    string header = "P3\n" + to_string(width) + " " + to_string(height) + "\n" + to_string(depth) + "\n";
    SaveOnFile(colorEdgeImg, width*3, height, argv[2], header);

    return 0;
}




void ToBeginLine(ifstream& file){
    /*The function move the pointer back at the beginning of the line,
     * by going back of 2 positions and then iterating the process until the \n character is found
     * if the cursor position is less then 2 then the cursor is moved to position 0 and the function returns*/

    if (file.tellg() < 2){
        file.seekg(0, ios_base::beg);
    }
    do {
        if(file.tellg() < 2) {
            file.seekg(0, ios_base::beg);
            return;
        }

        file.seekg(-2,ios_base::cur);
    }while(file.get() != '\n');

    return;
}

string AvoidComments(ifstream& file){
    /* Function avoid any line of comments by checking that the first character is not #,
     * when the line is not a comment line it moves back at it's beginning*/

    string line;
    do{
        getline(file, line);
        if(line[0] == '#')
        cout << line << "\n";
    }while(line[0] == '#');

    return line;
}

void SaveOnFile(vector<vector<int>>& img, int width, int height, char name[], string header){
    ofstream output_file(name, ios::out | ios::binary);
    output_file << header;
    for(int h = 0; h < height; h++){
        for(int w = 0; w < width; w++){
            output_file << img[h][w] << " ";
        }
        output_file << endl;
    }
    output_file.close();
}

edgeGradient EdgeFilter(vector<vector<int>>& img, int width, int height, int w, int h){


    vector<vector<int>> imageMatrix(3, vector<int>(3));
    if(w != 0 && w != width-1 && h != 0 && h != height-1){

        for(int row = h - 1; row <=h+1; row++){
            for(int column = w- 1; column <= w+1; column++){
                imageMatrix[row - h + 1][column - w + 1] = img[row][column];
            }

        }
        return EdgeFilterCalculation(imageMatrix);
    }else{
        int validRow, validColumn;
        validRow = max(min(h, height-2), 1);
        validColumn = max(min(w, width -2), 1);

        return EdgeFilter(img, width, height, validColumn, validRow);
    }


}

edgeGradient EdgeFilterCalculation(vector<vector<int>>& img){

    vector<vector<int>> vFilter{{+1,+2,+1},
                                {0,0,0},
                                {-1,-2,-1}};
    vector<vector<int>> hFilter{{-1,0,1},{-2,0,2},{-1,0,1}};


    double gradientX = 0;
    double gradientY = 0;


    for(int r = 0; r < 3; r++){
        for(int c = 0; c < 3; c++){
            gradientX = gradientX + img[r][c] * hFilter[r][c];
            gradientY = gradientY + img[r][c] * vFilter[r][c];

        }
    }

    edgeGradient edgeInfo(gradientX, gradientY);

    return edgeInfo;
}

weights WeightsComputation(double angle, double magnitude){


    const double redVector = M_PI_2;
    const double greenVector = (7.0/6.0)* M_PI ;
    const double blueVector = 2 * M_PI - (M_PI/6.0);



    int redWeight = round(magnitude * cos(abs(redVector-angle)));
    int greenWeight = round(magnitude * cos(abs(greenVector-angle)));
    int blueWeight = round(magnitude * cos (abs(blueVector-angle)));

    if(redWeight < 0){
        redWeight = 0;
    }
    if(greenWeight < 0){
        greenWeight = 0;
    }
    if(blueWeight < 0){
        blueWeight = 0;
    }

    weights Weights;
    Weights.red = redWeight;
    Weights.green = greenWeight;
    Weights.blue = blueWeight;

    return Weights;

}





