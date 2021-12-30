#include <iostream>
#include <vector>

class IRtlReader
{
public:
    struct RasterSizeInfo
    {
        int width;
        int height;
        int channels;
        int bitDepath; //1ピクセル当たりのドット情報サイズ
    };
    virtual int Read(int heightToRead, unsigned char *data) = 0;
    virtual RasterSizeInfo GetRasterSizeInfo() = 0;
    virtual ~IRtlReader() {}
};

namespace
{
int CalcBugSizeToReadRtl(const IRtlReader::RasterSizeInfo &sizeInfo)
{
    int byteSizePerLinePerChannnel = (sizeInfo.width * sizeInfo.bitDepath + 7 ) >> 3;
    return byteSizePerLinePerChannnel * sizeInfo.channels;
}
}
class RtlReader : public IRtlReader
{
public:
    // bitDepth は1byte固定とする。dataの配列数は実描画部分のピクセル数と同等とする。
    RtlReader(int width, int height, const std::vector<std::vector<unsigned char>>& data)
    {
        dstData_ = data;
        sizeInfo_.bitDepath = 8;
        sizeInfo_.channels = data.size();
        sizeInfo_.height = height;
        sizeInfo_.width = width;
    }
    virtual int Read(int heightToRead, unsigned char* data) override
    {
        for(int row = 0; row < heightToRead; ++row) {
            std::vector<unsigned char> outputData;
            for(int ch = 0; ch < dstData_.size(); ++ch) {
                std::vector<unsigned char> bufPerCh(sizeInfo_.width);
                // コンストラクタで与えられた幅に合わせて実データを描画する。
                for(int i =0; i < sizeInfo_.width; ++i) {
                    if(dstData_[ch].size() < i) {
                        data[i] = 0x00;
                        continue;
                    }
                    bufPerCh[i] = dstData_[ch][i];
                }
                outputData.insert(outputData.end(), bufPerCh.begin(), bufPerCh.end());
            }
            for(int w = 0; w < outputData.size(); ++w) {
                data[row * sizeInfo_.width + w] = outputData[w];
            }
        }
        return 0;
    }
    virtual IRtlReader::RasterSizeInfo GetRasterSizeInfo() override
    {
        return sizeInfo_;
    }

private:
    std::vector<std::vector<unsigned char>> dstData_;
    RasterSizeInfo sizeInfo_;
};

class RtlComposer : public IRtlReader
{
public:
    RtlComposer(const std::vector<IRtlReader*> &readers) : readers_{readers}
    {
        Initialize();
    }
    virtual int Read(int heightToRead, unsigned char* data) override
    {
        //for(int row = 0; row < heightToRead; ++row) {
            std::vector<unsigned char> totalBuf;
            for(auto &reader : readers_) {
                int bufSize = CalcBugSizeToReadRtl(reader->GetRasterSizeInfo());
                std::vector<unsigned char> bufPerLayer(bufSize);
                reader->Read(1, bufPerLayer.data());
                totalBuf.insert(totalBuf.end(), bufPerLayer.begin(), bufPerLayer.end());
            }
            for(int i = 0; i < totalBuf.size(); ++i) {
                data[i] = totalBuf[i];
            }
        //}
        return 0;
    }
    virtual IRtlReader::RasterSizeInfo GetRasterSizeInfo() override
    {
        return sizeInfo_;
    }
private:
    std::vector<IRtlReader*> readers_;
    RasterSizeInfo sizeInfo_;
    void Initialize()
    {
        sizeInfo_ = readers_[0]->GetRasterSizeInfo();
        int totalCh = 0;
        for(auto &elem : readers_) {
            RasterSizeInfo info = elem->GetRasterSizeInfo();
            totalCh += info.channels;
        }
        sizeInfo_.channels = totalCh;
    }
};


int main()
{
    // width * bitdepth + 7 >> 3 -> 1行あたりに必要なバイト数
    std::vector<std::vector<unsigned char>> data;
    std::vector<unsigned char> ch1 {0x01, 0x02};
    data.push_back(ch1);
    std::vector<unsigned char> ch2 {0x04, 0x05};
    data.push_back(ch2);
    RtlReader reader1(5, 2, data);
    RtlReader reader2(5, 2, data);
    RtlComposer composer({&reader1, &reader2});
    std::vector<unsigned char> dummy(20);
    composer.Read(1, dummy.data());
    //reader1.Read(1, dummy.data());
    std::copy(dummy.begin(),dummy.end(), std::ostream_iterator<unsigned char>(std::cout, ","));
    std::cout << std::endl;
    auto info = composer.GetRasterSizeInfo();
    std::cout << "ch:" << info.channels << ", width x height : " << info.width << " x " << info.height << ", bitdepth : " << info.bitDepath << std::endl;
}