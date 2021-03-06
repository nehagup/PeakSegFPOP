load("timing.data.RData")
library(data.table)

timing.data[, minutes := seconds/60]

timing.stats <- timing.data[penalty < Inf, list(
  min.minutes=min(minutes),
  mean.minutes=mean(minutes),
  median.minutes=median(minutes),
  max.minutes=max(minutes),
  total.minutes=sum(minutes),
  min.peaks=min(peaks),
  max.peaks=max(peaks),
  min.megabytes=min(megabytes),
  max.megabytes=max(megabytes),
  penalties=.N,
  n.Inf=sum(errors==Inf),
  feasible.changes=sum(abs(diff(status=="feasible")))
  ), by=.(problem.dir, n.data)][order(n.data),]
levs <- timing.stats$problem.dir
timing.stats[, problem.fac := factor(problem.dir, levs)]
timing.data[, problem.fac := factor(problem.dir, levs)]
table(timing.stats$feasible.changes)
weird.problem.dirs <- timing.stats[1 < feasible.changes, problem.dir]
weird.timings <- timing.data[problem.dir %in% weird.problem.dirs, ]
weird.list <- split(weird.timings, weird.timings$problem.fac, drop=TRUE)
lapply(weird.list, function(dt)dt[, .(n.data, penalty, peaks, status, fn, fp, errors)])
H3K36me3_TDH <- timing.data[grepl("H3K36me3_TDH", problem.fac),]
H3K36me3_TDH[, list(min.errors=min(errors)), by=problem.fac][order(min.errors),]
H3K36me3_TDH.list <- split(H3K36me3_TDH, H3K36me3_TDH$problem.fac, drop=TRUE)
lapply(H3K36me3_TDH.list, function(dt)dt[, .(n.data, penalty, peaks, status, fn, fp, errors)])
H3K36me3_TDH["labels/H3K36me3_TDH_immune/samples/tcell/McGill0102/problems/chr11:96437584-134946516"==problem.fac,]
H3K36me3_TDH["labels/H3K36me3_TDH_other/samples/skeletalMuscleCtrl/McGill0019/problems/chr14:19000000-107289540"==problem.fac,]

## labels/H3K4me3_TDH_immune/samples/tcell/McGill0025/problems/chr3:93504854-194041961
## ran PeakSegFPOP for too many infeasible models.

## labels/H3K4me3_TDH_immune/samples/tcell/McGill0024/problems/chr10:60000-17974675
## has infeasible models for 38-128 and 268-Inf peaks... is that a problem?

## labels/H3K4me3_XJ_immune/samples/tcell/McGill0010/problems/chr1:30028082-103863906
## does not compute the lower penalty limit precisely...

refs <- data.table(
  label=c("1 minute", "1 hour", "4 hours"),
  minutes=c(1, 60, 4*60))
ggplot()+
  ggtitle("Time for single PeakSegFPOP runs (min/median/max)")+
  geom_text(aes(4.5, log10(minutes), label=label),
            color="grey",
            vjust=1.5,
            data=refs)+
  geom_hline(aes(yintercept=log10(minutes)),
             color="grey",
            data=refs)+
  geom_segment(aes(log10(n.data), log10(min.minutes),
                 xend=log10(n.data), yend=log10(max.minutes)),
             data=timing.stats)+
  geom_point(aes(log10(n.data), log10(median.minutes)),
             data=timing.stats)+
  xlab("log10(number of data to segment = lines in bedGraph file)")

gg.penalties <- ggplot()+
  ggtitle("Number of penalties to find target interval is constant")+
  ylab("Number of penalties for which PeakSegFPOP was run")+
  xlab("log10(number of data to segment = lines in bedGraph file)")+
  geom_point(aes(log10(n.data), penalties),
             data=timing.stats)
png("figure-timing-data-penalties.png")
print(gg.penalties)
dev.off()

refs <- data.table(
  label=c("1 minute", "1 hour", "1 day"),
  minutes=c(1, 60, 60*24))
gg.total <- ggplot()+
  ggtitle("Time to compute target interval is linear")+
  geom_text(aes(4.5, log10(minutes), label=label),
            color="grey",
            vjust=-0.5,
            data=refs)+
  geom_hline(aes(yintercept=log10(minutes)),
             color="grey",
            data=refs)+
  geom_point(aes(log10(n.data), log10(total.minutes)),
             data=timing.stats)+
  xlab("log10(number of data to segment = lines in bedGraph file)")
png("figure-timing-data-target.png")
print(gg.total)
dev.off()
