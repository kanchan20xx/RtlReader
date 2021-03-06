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
int CalcBufSizeToReadRtlPerLine(const IRtlReader::RasterSizeInfo &sizeInfo)
{
    int byteSizePerLinePerChannnel = (sizeInfo.width * sizeInfo.bitDepath + 7 ) >> 3;
    return byteSizePerLinePerChannnel * sizeInfo.channels;
};

struct PixelMap
{
    std::vector<std::vector<unsigned char>> data;
};

}
class RtlReader : public IRtlReader
{
public:
    // bitDepth は1byte固定とする。dataの配列数は実描画部分のピクセル数と同等とする。
    RtlReader(int width, int height, const std::vector<PixelMap>& data)
    {
        dstData_ = data;
        sizeInfo_.bitDepath = 8;
        sizeInfo_.channels = data.size();
        sizeInfo_.height = height;
        sizeInfo_.width = width;
        heightRead_ = 0;
    }
    virtual int Read(int heightToRead, unsigned char* data) override
    {
        for(int row = 0; row < heightToRead; ++row) {
            std::vector<unsigned char> outputData;
            for(int ch = 0; ch < dstData_.size(); ++ch) {
                std::vector<unsigned char> bufPerCh(sizeInfo_.width);
                // コンストラクタで与えられた幅に合わせて実データを描画する。
                for(int i =0; i < sizeInfo_.width; ++i) {
                    if(dstData_[ch].data[row + heightRead_].size() < i) {
                        bufPerCh[i] = 0x00;
                        continue;
                    }
                    bufPerCh[i] = dstData_[ch].data[row + heightRead_][i];
                }
                outputData.insert(outputData.end(), bufPerCh.begin(), bufPerCh.end());
            }
            for(int w = 0; w < outputData.size(); ++w) {
                data[row * sizeInfo_.width * dstData_.size() + w] = outputData[w];
            }
        }
        heightRead_ += heightToRead; 
        return 0;
    }
    virtual IRtlReader::RasterSizeInfo GetRasterSizeInfo() override
    {
        return sizeInfo_;
    }

private:
    std::vector<PixelMap> dstData_;

    RasterSizeInfo sizeInfo_;
    int heightRead_;
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
        int bufSizePerLine = CalcBufSizeToReadRtlPerLine(GetRasterSizeInfo());
        for(int row = 0; row < heightToRead; ++row) {
            std::vector<unsigned char> totalBuf;
            for(auto &reader : readers_) {
                int bufSize = CalcBufSizeToReadRtlPerLine(reader->GetRasterSizeInfo());
                std::vector<unsigned char> bufPerLayer(bufSize);
                reader->Read(1, bufPerLayer.data());
                totalBuf.insert(totalBuf.end(), bufPerLayer.begin(), bufPerLayer.end());
            }
            for(int i = 0; i < totalBuf.size(); ++i) {
                data[row * bufSizePerLine + i] = totalBuf[i];
            }
        }
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
    std::vector<PixelMap> srcPixs;
    PixelMap srcPixsCh1;
    PixelMap srcPixsCh2;
    std::vector<unsigned char> ch1 {0x01, 0x02};
    srcPixsCh1.data.push_back(ch1);
    srcPixsCh1.data.push_back(ch1);
    std::vector<unsigned char> ch2 {0x04, 0x05};
    srcPixsCh2.data.push_back(ch2);
    srcPixsCh2.data.push_back(ch2);
    srcPixs.push_back(srcPixsCh1);
    srcPixs.push_back(srcPixsCh2);
    RtlReader reader1(5, 2, srcPixs);
    std::vector<unsigned char> dummy(40);
    RtlReader reader2(5, 2, srcPixs);
    RtlComposer composer({&reader1, &reader2});
    composer.Read(2, dummy.data());
    std::copy(dummy.begin(),dummy.end(), std::ostream_iterator<unsigned char>(std::cout, ","));
    std::cout << std::endl;
    auto info = composer.GetRasterSizeInfo();
    std::cout << "channel : " << info.channels << ", width x height : " << info.width << " x " << info.height << ", bitdepth : " << info.bitDepath << std::endl;
}