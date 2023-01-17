#include <iostream>
#include <fstream>
#include <cmath>
using namespace std;
#define DEBUG 1

string ref_list[10001];
bool hit_or_miss[10001] = {0}; // hit:1, miss:0

int main(int argc, char *argv[])
{
    // argv[0]: ./project
    if (argc != 4)
    {
        throw "Please Input: ./project cache.org reference.lst index.rpt\n";
        return 0;
    }

    fstream file;
    string cache_file = argv[1];
    string ref_file = argv[2];
    string output_file = argv[3];

    // read cache.org
    file.open(cache_file, ios::in);
    if (!file)
    {
        // "character error (stream)"
        cerr << "Cannot Open File: " << cache_file << endl;
        exit(1);
    }

    string header;
    int data;
    int data_num = 0;
    int data_list[4] = {0};
    // 讀至檔案結尾則傳回０
    while (file >> header >> data)
    {
        data_list[data_num++] = data;
    }
    file.close();

    int address_bits = data_list[0];
    int block_size = data_list[1];
    int cache_sets = data_list[2];
    int associativity = data_list[3];
    // calculate bits
    int offset_bit_count = log2(block_size);
    int index_bit_count = log2(cache_sets);
    int index_bit[128] = {0}; // just give larger size
    int tag_bit_count = address_bits - index_bit_count - offset_bit_count;

#if (DEBUG)
    cout << "tag_count = " << tag_bit_count << endl;
#endif

    for (int i = offset_bit_count, j = 0; j < index_bit_count; i++, j++)
    {
        index_bit[j] = i;
#if (DEBUG)
        cout << j << ": " << i << endl;
#endif
    }

#if (DEBUG)
    cout << "address_bits: " << address_bits << endl;
    cout << "block_size: " << block_size << endl;
    cout << "cache_sets: " << cache_sets << endl;
    cout << "associativity: " << associativity << endl;
    cout << "offset_bit_count: " << offset_bit_count << endl;
    cout << "index_bit_count: " << index_bit_count << endl;
#endif

    // read reference.lst
    file.open(ref_file, ios::in);
    if (!file)
    {
        cerr << "Cannot Open File: " << ref_file << endl;
        exit(1);
    }

    int ref_num = 0;
    // string ref_list[512]; // change to global def
    bool cout_start = false;
    while (file >> header)
    {
        ref_list[ref_num] = header;
        if (!cout_start)
        {
            file >> header;
            ref_list[ref_num] += " " + header;
            cout_start = true;
        }

#if (DEBUG)
        cout << "ref_list[" << ref_num << "] " << ref_list[ref_num] << endl;
#endif
        ref_num++;
        if (header == ".end")
        {
#if (DEBUG)
            cout << "success break\n";
#endif
            break;
        }
    }
    file.close();

    /********************* cache simulation **************************/
    int miss_count = 0;
    string cache[cache_sets][associativity];
    bool NRU_list[cache_sets][associativity];
    for (int i = 0; i < cache_sets; ++i)
        for (int j = 0; j < associativity; ++j)
            NRU_list[i][j] = 1;

    for (int i = 1; i < ref_num - 1; ++i)
    {
        string idxS;
        string tag;
        // calculate tag
        int j = 0;
        for (int k = 0; k < tag_bit_count; j++, k++)
        {
            tag += ref_list[i][j]; // ref_list : string
        }
#if (DEBUG)
        cout << "---------" << endl;
        cout << "ref:" << i << " = " << ref_list[i] << endl;
        cout << "j: " << j << endl;
        cout << "tag = " << tag << endl;
        cout << "---------" << endl;
#endif
        // calculate index
        for (int k = 0; k < index_bit_count; j++, k++)
        {
            idxS += ref_list[i][j];
#if (false)

            cout << "---------" << endl;
            cout << "ref:" << i << " = " << ref_list[i] << endl;
            cout << " j = " << j << endl;
            cout << " idxS = " << idxS << endl;
            cout << "---------" << endl;
#endif
        }
#if (DEBUG)
        cout << "idxS = " << idxS << endl;
        cout << "tag = " << tag << endl;
#endif
        // convert string to integer with base 2
        int idx = stoi(idxS, 0, 2);
#if (DEBUG)
        cout << "idx = " << idx << endl;
        cout << "---------" << endl;
#endif

        bool hit = false;
        for (int k = 0; k < associativity; k++)
        {
            // 一個一個去比對，比對成功 => hit
            if (cache[idx][k] == tag)
            {
                hit_or_miss[i] = true;
                NRU_list[idx][k] = 0;
                hit = true;
                break;
            }
        }
        if (hit)
        {
            continue;
        }

        hit_or_miss[i] = false;
        miss_count++;
        // replacement policy: LRU
        for (int a = 0; a < associativity; a++)
        {
            if (NRU_list[idx][a] == 1)
            {
                NRU_list[idx][a] = 0;
                cache[idx][a] = tag;
                break;
            }
            // flush
            else if (a == associativity - 1 && NRU_list[idx][a] == 0)
            {
                for (int a = 0; a < associativity; ++a)
                {
                    NRU_list[idx][a] = 1;
                }
                cache[idx][0] = tag;
                NRU_list[idx][0] = 0;
            }
        }
    }

    // write index.rpt
    file.open(output_file, ios::out);
    if (!file)
    {
        cerr << "Cannot Open File: " << output_file << endl;
        exit(1);
    }

    file << "Address bits: " << address_bits << endl;
    file << "Cache sets: " << cache_sets << endl;
    file << "Associativity: " << associativity << endl;
    file << "Block size: " << block_size << endl;
    file << endl;

    file << "Indexing bit count: " << index_bit_count << endl;
    file << "Indexing bits:";
    for (int i = index_bit_count - 1; i >= 0; i--)
    {
        file << " " << index_bit[i];
    }
    file << endl;
    file << "Offset bit count: " << offset_bit_count << endl;
    file << endl;

    for (int i = 0; i < ref_num; ++i)
    {
        string tmp;

        if (i == 0 | i == ref_num - 1)
        {
            tmp = "";
        }
        else if (hit_or_miss[i] == true)
        {
            tmp = " hit";
        }
        else
        {
            tmp = " miss";
        }

        file << ref_list[i] << tmp << endl;
    }
    file << endl;
    file << "Total cache miss count: " << miss_count << endl;
    file.close();
    return 0;
}
