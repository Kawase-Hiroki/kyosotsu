set terminal pngcairo
set output "image/out_disc_satis.png"
set datafile separator ","
set xlabel "Disclosure"
set ylabel "Matches"
plot "male_data.csv" using 3:6 with points title "Male", "female_data.csv" using 3:6 with points title "Female"
system("echo plot finished")
system("ls -lh image/out_disc_satis.png")