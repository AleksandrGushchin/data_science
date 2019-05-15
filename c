#include <iostream>
#include <algorithm>
#include <atomic>
#include <thread>
#include <string.h>

using namespace std;
using intl = uint64_t;
using FILE_pointer = unique_ptr<FILE, void (*)(FILE*)>;

const int amount = 1 << 18;
char *temp_file = "temp_file.bin";
char *input_file = "input.bin";
char *name_output = "output.bin";
char *output_first_file = "output1.bin";
char *output_second_file = "output2.bin";

void destructor(FILE*p)
    { fclose(p); }


// считываю по 2^17 чисел (по 1 МБ), сортирую их, и закидываю в файл tmp.bin
// возвращает количество таких считанных блоков
int first_read()
{
    unique_ptr<intl[]> data(new intl [amount]);
    FILE_pointer  input  (fopen(input_file, "rb"),  destructor);
    FILE_pointer  output (fopen(temp_file, "wb"),   destructor);

    int read_numbers = 0, i = 0;
    do
    {
        read_numbers = fread(data.get(), sizeof(intl), amount, input.get());
        i++;
        sort(data.get(), data.get() + read_numbers);
        fwrite(data.get(), sizeof(intl), read_numbers, output.get());
    }
    while (read_numbers == amount);
    return i;
}

// сортировка слиянием, подаю номер чанка, размер, имя файла, где числа есть, и файл, куда записывать уже отсортированное
// сортирует 2 куска по size_of_chunk МБ
void mergesort(const int beg, const int size_of_chunk, const char *name_output, FILE* const output)
{
    FILE_pointer  input1 (fopen(name_output, "rb"), destructor);
    FILE_pointer  input2 (fopen(name_output, "rb"), destructor);

    unique_ptr<intl []> first_inp  (new intl [amount]);
    unique_ptr<intl []> second_inp (new intl [amount]);
    unique_ptr<intl []> merge      (new intl [2 * amount]);

    //в обоих файлах считали все до нужного чанка, остальное забыли
    //for (int i = 0; i < beg; i++)
    //{
    fread(first_inp.get(),  sizeof(intl), amount * beg, input1.get());
    fread(second_inp.get(), sizeof(intl), amount * beg, input2.get());
    //}

    //считываем числа
    for (int i = 0; i <= size_of_chunk; i++)
    {
        fread(second_inp.get(), sizeof(intl), amount, input2.get());
    }

    fread(first_inp.get(), sizeof(intl), amount, input1.get());
    int numbers_read = fread(second_inp.get(), sizeof(intl), amount, input2.get());

    intl *first_cur = first_inp.get(), *sec_cur = second_inp.get();
    //r1 - указатель на текущий сравниваемый элемент в массиве q1
    int i1 = 1, i2 = 1; // количество считанных чанков
    bool who_was_end = 0;
    do
    {
        if (*first_cur - *(first_inp.get()) == amount)
        {
            i1++;
            fread(first_inp.get(), sizeof(intl), amount, input1.get());
            first_cur = first_inp.get();
        }
        if (*sec_cur  - *(q2.get()) == numbers_read)
        {
            i2++;
            numbers_read = fread(q2.get(), sizeof(type), amount, input2.get());
            sec_cur  = q2.get();
        }
        int how_comp = 0, now1 = 0, now2 = 0;
        while (now1 < amount && now2 < amount)
        {
            auto merge_array = merge.get();
            if (*first_cur < *r2)
            {
                merge_array[how_comp++] = *(first_cur++);
                now1++;
            }
            else
            {
                merge_array[how_comp++] = *(sec_cur ++);
                now2++;
            }
        }
        finished = now1 != amount;

        fwrite(merge.get(), sizeof(intl), how_comp, output);
    }
    while (!finished && !(i1 == size_of_chunk && !finished) && !(i2 == size_of_chunk && finished));

    if (finished)
    {
        fwrite(first_cur , sizeof(intl), amount - (*first_cur - *(first_inp.get())), output);
        while (i1 != size_of_chunk)
        {
            fread(first_inp.get(), sizeof(intl), amount, input1.get());
            i1++;
            fwrite(first_inp.get(), sizeof(intl), amount, output);
        }
    }
    else
    {
        fwrite(sec_cur , sizeof(intl), amount, output);
        while (i2 != size_of_chunk)
        {
            fread(second_inp.get(), sizeof(intl), amount, input2.get());
            i2++;
            fwrite(second_inp.get(), sizeof(intl), amount, output);
        }
    }
    return;
}

void merge_file(bool flag)
{
    char *name1 = strdup(output_first_file), *name2 = strdup(output_second_file);
    if (!flag)
        swap(name1, name2);

    FILE_pointer  output1 (fopen(name1, "rb"),      destructor);
    FILE_pointer  output2 (fopen(name2, "rb"),      destructor);
    FILE_pointer  output (fopen(name_output, "wb"), destructor);
    unique_ptr<intl []> buf (new intl [amount]);
    int how_read;
    while (how_read = fread(buf.get(), sizeof(intl), amount, output1.get()))
    {
        fwrite(buf.get(), sizeof(intl), how_read, output.get());
    }
    while (how_read = fread(q.get(), sizeof(intl), amount, output2.get()))
    {
        fwrite(buf.get(), sizeof(intl), how_read, output.get());
    }
    return;
}

int main()
{
    thread thread1, thread2;
    int n = first_read();
    int i = (int)log2(n) + 1, size = 1;
    n /= 2;
    //слияния по 2 соседних блока чисел
    for (int j = 0; j < i; j++)
    {
        FILE_pointer  output1 (fopen(output_first_file, "wb"),destructor);
        FILE_pointer  output2 (fopen(output_second_file, "wb"),destructor);

        for (int k = 1; k <= n; k += 2)
        {
            thread1 = thread(mergesort, (k-1) * 2 * size, size, temp_file, output1.get());
            thread2 = thread(mergesort,     k * 2 * size, size, temp_file, output2.get());
            thread1.join();
            thread2.join();
        }
        size <<= 1;
        merge_file(!(n & 1));
    }

    return 0;
}
