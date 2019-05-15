#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>
using namespace std;
using type = uint64_t;
const int amount = 1<<17;
const char *name_tmp = "tmp.bin";
const char *name_input = "input.bin";
const char *name_output = "output.bin";
const char *name_output1 = "output1.bin";
const char *name_output2 = "output2.bin";
atomic<bool> ok {true};

void deleter(FILE*p)
{
    fclose(p);
};
using FILE_ptr = std::unique_ptr<FILE, void (*)(FILE*)>;

using ptr = std::unique_ptr<type[]>;

// считываю по 2^17 чисел (по 1 МБ), сортирую их, и закидываю в файл tmp.bin
// возвращает количество таких считанных блоков
int first_read()
{
    ptr q(new type [amount]);
    FILE_ptr input (fopen(name_input, "rb"),deleter);
    if (input == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return 0;
    }
    FILE_ptr output (fopen(name_tmp, "wb"),deleter);
    if (output == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return 0;
    }
    int now = 0, i = 0;
    do
    {
        now = fread(q.get(), sizeof(type), amount, input.get());
        i++;
        std::sort(q.get(), q.get()+now);
        fwrite(q.get(), sizeof(type), now, output.get());
    }
    while (now == amount);
    return i;
}

// сортировка слиянием, подаю номер чанка, размер, имя файла, где числа есть, и файл, куда записывать уже отсортированное
// сортирует 2 куска по size_of_chunk МБ
void mergesort(const int beg, const int size_of_chunk, const char *name_output, FILE* const output, bool bol)
{
    if (!bol)
        return ;
    FILE_ptr input1 (fopen(name_output, "rb"),deleter);
    if (input1 == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return;
    }
    FILE_ptr input2 (fopen(name_output, "rb"),deleter);
    if (input2 == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return;
    }
    ptr q1(new type [amount]);
    ptr q2(new type [amount]);
    ptr q3(new type [2*amount]);
    for (int i = 1; i < beg; i++)
    {
        fread(q1.get(), sizeof(type), amount, input1.get());
        fread(q2.get(), sizeof(type), amount, input2.get());
    }
    for (int i = 0; i < size_of_chunk; i++)
    {
        fread(q2.get(), sizeof(type), amount, input2.get());
    }



    fread(q1.get(), sizeof(type), amount, input1.get());
    int how_read = fread(q2.get(), sizeof(type), amount, input2.get());
    type *r1 = q1.get(), *r2 = q2.get(); //r1 - указатель на текущий сравниваемый элемент в массиве q1
    int i1 = 1, i2 = 1; // количество считанных чанков
    bool who_was_end = 0;
    do
    {
        if (*r1 - *(q1.get()) == amount)
        {
            i1++;
            fread(q1.get(), sizeof(type), amount, input1.get());
            r1 = q1.get();
        }
        if (*r2 - *(q2.get()) == how_read)
        {
            i2++;
            how_read = fread(q2.get(), sizeof(type), amount, input2.get());
            r2 = q2.get();
        }
        int how_comp = 0, now1 = 0, now2 = 0;
        while (now1 < amount && now2 < how_read)
        {
            if (*r1 < *r2)
            {
                q3.get()[how_comp] = *r1;
                r1++;
                now1++;
            }
            else
            {
                q3.get()[how_comp] = *r2;
                r2++;
                now2++;
            }
        }
        if (now1 == amount)
            who_was_end = 0;
        else
            who_was_end = 1;

        fwrite(q3.get(), sizeof(type), how_comp, output);
    }
    while (!(how_read != amount && who_was_end) && !(i1 == size_of_chunk && !who_was_end) && !(i2 == size_of_chunk && who_was_end));

    if (who_was_end)
    {
        fwrite(r1, sizeof(type), amount - (*r1 - *(q1.get())), output);
        while (i1 != size_of_chunk)
        {
            fread(q1.get(), sizeof(type), amount, input1.get());
            i1++;
            fwrite(q1.get(), sizeof(type), amount, output);
        }
    }
    else
    {
        fwrite(r2, sizeof(type), how_read - (*r2 - *(q2.get())), output);
        while (i2 != size_of_chunk && how_read == amount)
        {
            how_read = fread(q2.get(), sizeof(type), amount, input2.get());
            i2++;
            fwrite(q2.get(), sizeof(type), how_read, output);
        }
    }
}

void merge_file(bool a)
{
    FILE_ptr output1 (fopen(name_output1, "rb"),deleter);
    if (output1 == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return;
    }
    FILE_ptr output2 (fopen(name_output2, "rb"),deleter);
    if (output2 == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return;
    }
    FILE_ptr output (fopen(name_output, "wb"),deleter);
    if (output == NULL)
    {
        printf("Cannot open file.\n");
        ok = false;
        return;
    }
    ptr q(new type [amount]);
    int how_read;
    if (a)
    {
        while (how_read = fread(q.get(), sizeof(type), amount, output1.get()))
        {
            fwrite(q.get(), sizeof(type), how_read, output.get());
        }
        while (how_read = fread(q.get(), sizeof(type), amount, output2.get()))
        {
            fwrite(q.get(), sizeof(type), how_read, output.get());
        }
    }
    else
    {
        while (how_read = fread(q.get(), sizeof(type), amount, output2.get()))
        {
            fwrite(q.get(), sizeof(type), how_read, output.get());
        }
        while (how_read = fread(q.get(), sizeof(type), amount, output1.get()))
        {
            fwrite(q.get(), sizeof(type), how_read, output.get());
        }
    }
}

int main()
{
    thread thread1, thread2;
    int n = first_read();
    if (!ok)
    {
        printf("Cannot open file.\n");
        return 0;
    }
    int pow = 1, i = 0;
    while (pow < n)
    {
        pow <<= 1;
        i++;
    }
    //слияния по 2 соседних блока чисел
    for (int j = 0; j < i; j++)
    {
        FILE_ptr output1 (fopen(name_output1, "wb"),deleter);
        FILE_ptr output2 (fopen(name_output2, "wb"),deleter);
        if (output1 == NULL)
        {
            printf("Cannot open file.\n");
            return 0;
        }
        if (output2 == NULL)
        {
            printf("Cannot open file.\n");
            return 0;
        }

        for (int k = 1; k <= n/2 && ok; k+=2)
        {
            thread1 = thread(mergesort, (k-1)*2*(1<<j), 1<<j, name_tmp, output1.get(), k<n/2);
            thread2 = thread(mergesort, k*2*(1<<j), 1<<j, name_tmp, output2.get(), (k-1)<n/2);
            thread1.join();
            thread2.join();
        }
        if (!ok)
        {
            printf("Cannot open file.\n");
            return 0;
        }
        if ((n/2)&1)
            merge_file(false);  //последним записывали в output1
        else
            merge_file(true);
        rename(name_output, name_tmp);
    }


    rename(name_tmp, name_output);
    //результат в output.bin
    return 0;
}
