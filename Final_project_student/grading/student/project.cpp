#include <iostream>
#include <fstream>
#include <cmath>
#include <list>
#include <unordered_map>
#include <vector>
#include <cstdlib>
#include <algorithm>
using namespace std;
#define DEBUG 0
#define OPTDEBUG 0

class LRUCache
{

private:
    int capacity;
    list<int> cache;
    unordered_map<int, list<int>::iterator> map;

public:
    LRUCache();
    LRUCache(int capacity_)
    {
        capacity = capacity_;
    }

    // return false if doesn't exist in cache.
    bool get(int key)
    {
        auto it = map.find(key); // return iterators
        if (it == map.end())     // not found
        {
            return false;
        }
        // found data, need to update it(put the data to the end of cache list)
        // Ex. 2 3 4 5 key 6 7
        // => 2 3 4 5 6 7 key

        cache.splice(cache.end(), cache, it->second);
        return true;
    }

    // displays contents of cache in Reverse Order
    void display()
    {
        for (auto it = cache.rbegin(); it != cache.rend(); ++it)
        {
            cout << *it << " ";
        }
    }

    void put(int key)
    {
        // 越久以前access的會在最前面，不斷把新的data放在最後面(push_back)
        // 當cache list滿時，從最前面開始pop

        if (cache.size() == capacity)
        {
            int first_key = cache.front();
            cache.pop_front();
            map.erase(first_key);
        }

        cache.push_back(key);
        // if key exists, cover it with new data(at the end of cache list)
        // unordered_map<int, list<int>::iterator> map;
        map[key] = --cache.end();
    }
};
string ref_list[10001];
// string *ref_list = (string*)malloc(sizeof(string)*);
bool hit_or_miss[10001] = {0}; // hit: 1, miss: 0

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
    int *index_bit = (int *)malloc(sizeof(int) * index_bit_count);
    vector<int> index_opt;
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

    // idx starts form MSB
    // optimize index
    int set_bits = address_bits - offset_bit_count;
    int opt_idx_cnt = 0;
    int *one_cnt = (int *)malloc(sizeof(int) * set_bits); // Ai bit 有幾個為1（在全部資料中）{A0(MSB), A1, A2}
    double *quality = (double *)malloc(sizeof(double) * set_bits);

    // caculate quality, find first bit
    double qua_max = -1;
    int max_idx = 0;

// ref_list idx 01234....
#if (OPTDEBUG)
    cout << "calculate first quality\n";
#endif

    // 順序跟論文顛倒，這裡是從MSB到LSB
    for (int i = 0; i < set_bits; i++)
    {

        int tmp_one = 0;
        for (int k = 1; k < ref_num - 1; k++)
        {
            if (ref_list[k][i] == '1')
            {
                tmp_one++;
#if (OPTDEBUG)
                cout << k << "=>" << tmp_one << endl;
#endif
            }
        }
        int tmp_zero = (ref_num - 2) - tmp_one;

        double tmp_max = max(tmp_zero, tmp_one);
        double tmp_min = min(tmp_zero, tmp_one);
        quality[i] = tmp_min / tmp_max;
#if (OPTDEBUG)
        cout << "tmp     " << tmp_one << ": " << tmp_zero << endl;
        cout << "Quality " << i << ": " << quality[i] << endl;
#endif

        if (max(qua_max, quality[i]) == quality[i]) // update (cmp=quality)
        {
            qua_max = quality[i];
            max_idx = i;
        }
    }
    // 先挑最後面ㄉ
    index_opt.push_back(max_idx);
    opt_idx_cnt++;

#if (OPTDEBUG)
    cout << "OPT_IDX: " << max_idx << endl;
#endif
    while (opt_idx_cnt != index_bit_count)
    {
        qua_max = -1;
        double *cor = (double *)malloc(sizeof(double) * set_bits);
        int prev_idx = index_opt.back();
        for (int i = 0; i < set_bits; i++)
        {
            if (i == prev_idx)
            {
                quality[i] = 0;
                continue;
            }
            int tmp_ident = 0;
            for (int k = 1; k < ref_num - 1; k++)
            {
                if (ref_list[k][max_idx] == ref_list[k][i])
                {
                    tmp_ident++;
                    // cout << k << "=>" << tmp_ident << endl;
                }
            }
            int tmp_diff = (ref_num - 2) - tmp_ident;
            // cout << "tmp     " << tmp_one << ": " << tmp_zero << endl;
            double tmp_max = max(tmp_ident, tmp_diff);
            double tmp_min = min(tmp_ident, tmp_diff);
            quality[i] *= tmp_min / tmp_max;

            cout << i << ": " << quality[i] << endl;

            if (max(qua_max, quality[i]) == quality[i]) // update (cmp=quality)
            {
                qua_max = quality[i];
                max_idx = i;
            }
        }
        index_opt.push_back(max_idx);
        opt_idx_cnt++;
    }
    // 升序
    sort(index_opt.begin(), index_opt.end());
    // sort 完 MSB to LSB

#if (OPTDEBUG)
    cout << "print index vector\n";
    for (auto it = index_opt.begin(); it != index_opt.end(); ++it)
    {
        cout << *it << " ";
    }
    cout << endl;
#endif

    // cache implementation

    // initialize Cache
    vector<LRUCache> Cache;

    for (int i = 0; i < cache_sets; i++)
    {
        LRUCache tmp2 = LRUCache(associativity);
        Cache.push_back(tmp2);
    }
    int miss_count = 0;
    for (int i = 1; i < ref_num - 1; i++)
    {
        string idxS;
        string tagS;
        // calculate tag
        // *********************prob
        for (int j = 0; j < set_bits; j++)
        {
            auto it = find(index_opt.begin(), index_opt.end(), j);
            if (it != index_opt.end()) // found
            {
                idxS += ref_list[i][j];
            }
            else
            {
                tagS += ref_list[i][j];
            }
        }
        cout << "finish produce tagS and idxS\n";
#if (DEBUG)
        cout << "---------" << endl;
        cout << "ref:" << i << " = " << ref_list[i] << endl;
        cout << "j: " << j << endl;
        cout << "tag = " << tagS << endl;
        cout << "---------" << endl;
        cout << "idxS = " << idxS << endl;
        cout << "tag = " << tagS << endl;
#endif
        // convert string to integer with base 2
        int idx = stoi(idxS, 0, 2);
        int tag = stoi(tagS, 0, 2);
#if (DEBUG)
        cout << "idx = " << idx << endl;
        cout << "---------" << endl;
#endif

        if (Cache[idx].get(tag)) // retrun true if data already exists in list
        {
            hit_or_miss[i] = true;
        }
        else // doesn't exist in cache
        {
            miss_count++;
            hit_or_miss[i] = false;
            Cache[idx].put(tag);
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
    // 要處理 儲存的值是MSB to LSB
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
