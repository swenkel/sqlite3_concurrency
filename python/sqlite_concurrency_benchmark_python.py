import multiprocessing as mp
import os
import pickle
import random
import sqlite3
import string
import sys
import time

import numpy as np


def remove_db():
    if os.path.exists('./test_db.sqlite3'):
        os.remove('./test_db.sqlite3')


def generate_empty_db():
    remove_db()
    res = True
    try:
        db_conn = sqlite3.connect("./test_db.sqlite3")
        db_curs = db_conn.cursor()
        db_curs.execute("CREATE TABLE test_table(\
            ID          INTEGER    PRIMARY KEY AUTOINCREMENT, \
            NAME        TEXT       NOT NULL,\
            IMAGE       TEXT       NOT NULL,\
            SOME_REAL   REAL       NOT NULL,\
            TS          TIMESTAMP DEFAULT CURRENT_TIMESTAMP)")
        db_conn.commit()
    except Exception as error:
        print(error)
        res = False

    return res


def rand_str(n):
    """Generates a random string of length n
    """
    rand_string = ""
    chars = "0123456789"+string.ascii_letters
    for i in range(n):
        rand_string += random.choice(chars)
    return rand_string


def add_data_to_db(runtimes_shared,
                   interrupt = False):
    db_conn = sqlite3.connect("./test_db.sqlite3")
    db_curs = db_conn.cursor()
    db_conn.execute("PRAGMA busy_timeout = 3000000")
    name = rand_str(20)
    image = rand_str(12000)
    number = random.random()

    start_time = time.time()
    for i in range(2000):
        db_curs.execute("INSERT INTO test_table(\
                NAME,IMAGE,SOME_REAL) \
            VALUES (?,?,?)",
            (name,image,number,))
        db_conn.commit()
        if interrupt:
            time.sleep(0.0002)
    runtime = (time.time()-start_time)/2000
    db_conn.close()
    runtimes_shared.put(runtime)


def main():
    print('=' * 72)
    print('Running SQLite concurrency benchmark (Python)')
    
    if os.path.exists('../results_total_average.csv'):
        total_average = open('../results_total_average.csv', 'a')
    else:
        total_average = open('../results_total_average.csv', 'w')
        total_average.write('language,threads,runtime(s)\n')
        total_average.flush()
    
    if os.path.exists('../results_thread_average.csv'):
        thread_average = open('../results_thread_average.csv', 'a')
    else:
        thread_average = open('../results_thread_average.csv', 'w')
        thread_average.write('language,threads,runtime(s)\n')
        thread_average.flush()

    res = generate_empty_db()
    if res:
        no_parallel_workers = [1,2,4,8]
        for n_jobs in no_parallel_workers:
            print('Benchmarking {} workers'.format(n_jobs))
            res = generate_empty_db()
            if res:
                worker_count = 0
                runtimes_shared = mp.Queue(-1)
                threads = {}
                for i in range(n_jobs):
                    threads[i] = mp.Process(target=add_data_to_db,
                                            args=(runtimes_shared,))
                start_time = time.time()
                for i in range(n_jobs):
                    threads[i].start()
                while worker_count < n_jobs:
                    if not runtimes_shared.empty():
                        duration = runtimes_shared.get()
                        worker_count += 1
                        thread_average.write('Python,{},{}\n'.format(
                            n_jobs, duration))
                        thread_average.flush()
                duration = (time.time()-start_time)/(n_jobs*2000)
                print(duration)
                total_average.write('Python,{},{}\n'.format(
                    n_jobs, duration))
                total_average.flush()

                for i in range(n_jobs):
                    try:
                        threads[i].join()
                    except Exception as error:
                        print(error)
        total_average.close()
        thread_average.close()
        sys.exit(0)
    else:
        remove_db()
        sys.exit(1)
    print('=' * 72)

if __name__ == '__main__':
    main()
