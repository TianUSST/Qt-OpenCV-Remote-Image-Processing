#pragma once
#include<opencv2/core.hpp>
#include<opencv2/opencv.hpp>
#include<sstream>
#include"base64.h"

class imageProcessor
{
    public:
        // static cv::Mat process(const cv::Mat &decodedImg,const std::string& rawCommand);//在内部做分割字符
        cv::Mat process(const cv::Mat &decodedImg,const std::string& rawCommand);//在内部做分割字符
        void getAddImage(const cv::Mat image);

    private:
        cv::Mat histTemplate;
        

//分割客户端传来的数据
    static std::pair<std::string, std::vector<std::string>> splitCommand(const std::string& rawCommand);

        
};