#include <iostream>
#include <vector>

class IRtlReader
{
public:
    virtual int Read(int heightToRead, unsigned char *data) = 0;
};

class RtlReader : public IRtlReader
{
public:
    // bitDepth は4固定とする。dataの配列数は実描画部分のピクセル数と同等とする。
    RtlReader(int width, int height, const std::vector<std::vector<unsigned char>>& data)
    {
        dstData_ = data;
        drawWidth_ = width;
    }
    virtual int Read(int heightToRead, unsigned char* data) override
    {
        // コンストラクタで与えられたデータを結合する。
        std::vector<unsigned char> outputData;
        for(int ch = 0; ch < dstData_.size(); ++ch) {
            outputData.insert(outputData.end(), dstData_[ch].begin(), dstData_[ch].end());
        }
        for(int row = 0; row < heightToRead; ++row) {
            // コンストラクタで与えられた幅に合わせて実データを描画する。
            for(int i =0; i < drawWidth_; ++i) {
                if(outputData.size() < i) {
                    data[i] = 0x00;
                    continue;
                }
                data[row * drawWidth_ + i] = outputData[i];
            }
        }
        return 0;
    }

private:
    std::vector<std::vector<unsigned char>> dstData_;
    int drawWidth_;
};

int main()
{
    // width * bitdepth + 7 >> 3 -> 1行あたりに必要なバイト数
    std::vector<std::vector<unsigned char>> data;
    std::vector<unsigned char> ch1 {0x01, 0x02};
    data.push_back(ch1);
    std::vector<unsigned char> ch2 {0x04, 0x05};
    data.push_back(ch2);
    RtlReader reader(5, 2, data);
    std::vector<unsigned char> dummy(10);
    reader.Read(2, dummy.data());
    std::copy(dummy.begin(),dummy.end(), std::ostream_iterator<unsigned char>(std::cout, ","));
}