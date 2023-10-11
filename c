import &#39;package:flutter/material.dart&#39;;
import &#39;package:habo/habits/calendar_header.dart&#39;;
import &#39;package:habo/habits/empty_list_image.dart&#39;;
import &#39;package:habo/habits/habit.dart&#39;;
import &#39;package:habo/habits/habits_manager.dart&#39;;
import &#39;package:provider/provider.dart&#39;;
class CalendarColumn extends StatelessWidget {
  const CalendarColumn({super.key});
  @override
  Widget build(BuildContext context) {
    List&lt;Habit&gt; calendars = Provider.of&lt;HabitsManager&gt;(context).getAllHabits;
    return Column(
      children: &lt;Widget&gt;[
        const Padding(
          padding: EdgeInsets.fromLTRB(18, 10, 18, 10),
          child: CalendarHeader(),
        ),
        Expanded(
          child: (calendars.isNotEmpty)
              ? ReorderableListView(
                  physics: const BouncingScrollPhysics(),
                  padding: const EdgeInsets.fromLTRB(0, 0, 0, 120),
                  children: calendars
                      .map(
                        (index) =&gt; Container(
                          key: ObjectKey(index),
                          child: index,
                        ),
                      )
                      .toList(),
                  onReorder: (oldIndex, newIndex) {
                    Provider.of&lt;HabitsManager&gt;(context, listen: false)
                        .reorderList(oldIndex, newIndex);
                  },
                )
              : const EmptyListImage(),
        ),
      ],
    );
  }
}
import &#39;package:flutter/material.dart&#39;;
import &#39;package:habo/settings/settings_manager.dart&#39;;
import &#39;package:provider/provider.dart&#39;;

class CalendarHeader extends StatelessWidget {
  const CalendarHeader({super.key});
  @override
  Widget build(BuildContext context) {
    final settingsManager = Provider.of&lt;SettingsManager&gt;(context);
    return SizedBox(
      height: 30.0,
      child: ListView.builder(
        shrinkWrap: true,
        scrollDirection: Axis.horizontal,
        itemCount: settingsManager.getWeekStartList.length,
        itemBuilder: (
          BuildContext ctx,
          int index,
        ) {
          int start = settingsManager.getWeekStartEnum.index;
          int day = (start + index) % settingsManager.getWeekStartList.length;
          TextStyle tex = const TextStyle(fontSize: 18, color: Colors.grey);
          if (settingsManager.getWeekStartList[day] == &quot;Sa&quot; ||
              settingsManager.getWeekStartList[day] == &quot;Su&quot;) {
            tex = TextStyle(fontSize: 18, color: Colors.red[300]);
          }
          return SizedBox(
            width: (MediaQuery.of(context).size.width - 32) * 0.141,
            child: Center(
              child: Text(settingsManager.getWeekStartList[day], style: tex),
            ),
          );
        },
      ),
    );
  }
}
habits headerimport &#39;dart:collection&#39;;
import &#39;dart:convert&#39;;
import &#39;package:flutter/material.dart&#39;;
import &#39;package:flutter_file_dialog/flutter_file_dialog.dart&#39;;
import &#39;package:habo/constants.dart&#39;;
import &#39;package:habo/habits/habit.dart&#39;;
import &#39;package:habo/model/backup.dart&#39;;
import &#39;package:habo/model/habit_data.dart&#39;;
import &#39;package:habo/model/habo_model.dart&#39;;
import &#39;package:habo/notifications.dart&#39;;
import &#39;package:habo/statistics/statistics.dart&#39;;
class HabitsManager extends ChangeNotifier {
  final HaboModel _haboModel = HaboModel();
  final GlobalKey&lt;ScaffoldMessengerState&gt; _scaffoldKey =
      GlobalKey&lt;ScaffoldMessengerState&gt;();

  late List&lt;Habit&gt; allHabits = [];
  bool _isInitialized = false;
  Habit? deletedHabit;
  Queue&lt;Habit&gt; toDelete = Queue();
  void initialize() async {
    await initModel();
    await Future.delayed(const Duration(seconds: 5));
    notifyListeners();
  }
  resetHabitsNotifications() {
    resetNotifications(allHabits);
  }
  initModel() async {
    await _haboModel.initDatabase();
    allHabits = await _haboModel.getAllHabits();
    _isInitialized = true;
    notifyListeners();
  }
  GlobalKey&lt;ScaffoldMessengerState&gt; get getScaffoldKey {
    return _scaffoldKey;
  }
  void hideSnackBar() {
    _scaffoldKey.currentState!.hideCurrentSnackBar();
  }
  createBackup() async {
    try {
      var file = await Backup.writeBackup(allHabits);
      final params = SaveFileDialogParams(
        sourceFilePath: file.path,
        mimeTypesFilter: [&#39;application/json&#39;],
      );
      await FlutterFileDialog.saveFile(params: params);
    } catch (e) {
      showErrorMessage(&#39;ERROR: Creating backup failed.&#39;);
    }
  }
  loadBackup() async {
    try {
      const params = OpenFileDialogParams(
        fileExtensionsFilter: [&#39;json&#39;],
        mimeTypesFilter: [&#39;application/json&#39;],
      );
      final filePath = await FlutterFileDialog.pickFile(params: params);

      if (filePath == null) {
        return;
      }
      final json = await Backup.readBackup(filePath);
      List&lt;Habit&gt; habits = [];
      jsonDecode(json).forEach((element) {
        habits.add(Habit.fromJson(element));
      });
      await _haboModel.useBackup(habits);
      removeNotifications(allHabits);
      allHabits = habits;
      resetNotifications(allHabits);
      notifyListeners();
    } catch (e) {
      showErrorMessage(&#39;ERROR: Restoring backup failed.&#39;);
    }
  }
  resetNotifications(List&lt;Habit&gt; habits) {
    for (var element in habits) {
      if (element.habitData.notification) {
        var data = element.habitData;
        setHabitNotification(data.id!, data.notTime, &#39;Habo&#39;, data.title);
      }
    }
  }
  removeNotifications(List&lt;Habit&gt; habits) {
    for (var element in habits) {
      disableHabitNotification(element.habitData.id!);
    }
  }
  showErrorMessage(String message) {
    _scaffoldKey.currentState!.hideCurrentSnackBar();
    _scaffoldKey.currentState!.showSnackBar(
      SnackBar(
        duration: const Duration(seconds: 3),
        content: Text(message),
        behavior: SnackBarBehavior.floating,
        backgroundColor: HaboColors.red,
      ),
    );
  }
  List&lt;Habit&gt; get getAllHabits {
    return allHabits;
  }
  bool get isInitialized {
    return _isInitialized;
  }

  reorderList(oldIndex, newIndex) {
    if (newIndex &gt; oldIndex) {
      newIndex -= 1;
    }
    Habit moved = allHabits.removeAt(oldIndex);
    allHabits.insert(newIndex, moved);
    updateOrder();
    _haboModel.updateOrder(allHabits);
    notifyListeners();
  }
  addEvent(int id, DateTime dateTime, List event) {
    _haboModel.insertEvent(id, dateTime, event);
  }
  deleteEvent(int id, DateTime dateTime) {
    _haboModel.deleteEvent(id, dateTime);
  }
  addHabit(
      String title,
      bool twoDayRule,
      String cue,
      String routine,
      String reward,
      bool showReward,
      bool advanced,
      bool notification,
      TimeOfDay notTime,
      String sanction,
      bool showSanction,
      String accountant) {
    Habit newHabit = Habit(
      habitData: HabitData(
        position: allHabits.length,
        title: title,
        twoDayRule: twoDayRule,
        cue: cue,
        routine: routine,
        reward: reward,
        showReward: showReward,
        advanced: advanced,
        events: SplayTreeMap&lt;DateTime, List&gt;(),
        notification: notification,
        notTime: notTime,
        sanction: sanction,
        showSanction: showSanction,
        accountant: accountant,
      ),
    );
    _haboModel.insertHabit(newHabit).then(

      (id) {
        newHabit.setId = id;
        allHabits.add(newHabit);
        if (notification) {
          setHabitNotification(id, notTime, &#39;Habo&#39;, title);
        } else {
          disableHabitNotification(id);
        }
        notifyListeners();
      },
    );
    updateOrder();
  }
  editHabit(HabitData habitData) {
    Habit? hab = findHabitById(habitData.id!);
    if (hab == null) return;
    hab.habitData.title = habitData.title;
    hab.habitData.twoDayRule = habitData.twoDayRule;
    hab.habitData.cue = habitData.cue;
    hab.habitData.routine = habitData.routine;
    hab.habitData.reward = habitData.reward;
    hab.habitData.showReward = habitData.showReward;
    hab.habitData.advanced = habitData.advanced;
    hab.habitData.notification = habitData.notification;
    hab.habitData.notTime = habitData.notTime;
    hab.habitData.sanction = habitData.sanction;
    hab.habitData.showSanction = habitData.showSanction;
    hab.habitData.accountant = habitData.accountant;
    _haboModel.editHabit(hab);
    if (habitData.notification) {
      setHabitNotification(
          habitData.id!, habitData.notTime, &#39;Habo&#39;, habitData.title);
    } else {
      disableHabitNotification(habitData.id!);
    }
    notifyListeners();
  }
  String getNameOfHabit(int id) {
    Habit? hab = findHabitById(id);
    return (hab != null) ? hab.habitData.title : &quot;&quot;;
  }
  Habit? findHabitById(int id) {
    Habit? result;
    for (var hab in allHabits) {
      if (hab.habitData.id == id) {
        result = hab;
      }
    }
    return result;

  }
  deleteHabit(int id) {
    deletedHabit = findHabitById(id);
    allHabits.remove(deletedHabit);
    toDelete.addLast(deletedHabit!);
    Future.delayed(const Duration(seconds: 4), () =&gt; deleteFromDB());
    _scaffoldKey.currentState!.hideCurrentSnackBar();
    _scaffoldKey.currentState!.showSnackBar(
      SnackBar(
        duration: const Duration(seconds: 3),
        content: const Text(&quot;Habit deleted.&quot;),
        behavior: SnackBarBehavior.floating,
        action: SnackBarAction(
          label: &#39;Undo&#39;,
          onPressed: () {
            undoDeleteHabit(deletedHabit!);
          },
        ),
      ),
    );
    updateOrder();
    notifyListeners();
  }
  undoDeleteHabit(Habit del) {
    toDelete.remove(del);
    if (deletedHabit != null) {
      if (deletedHabit!.habitData.position &lt; allHabits.length) {
        allHabits.insert(deletedHabit!.habitData.position, deletedHabit!);
      } else {
        allHabits.add(deletedHabit!);
      }
    }
    updateOrder();
    notifyListeners();
  }
  Future&lt;void&gt; deleteFromDB() async {
    if (toDelete.isNotEmpty) {
      disableHabitNotification(toDelete.first.habitData.id!);
      _haboModel.deleteHabit(toDelete.first.habitData.id!);
      toDelete.removeFirst();
    }
    if (toDelete.isNotEmpty) {
      Future.delayed(const Duration(seconds: 1), () =&gt; deleteFromDB());
    }
  }
  updateOrder() {
    int iterator = 0;

    for (var habit in allHabits) {
      habit.habitData.position = iterator++;
    }
  }
  Future&lt;AllStatistics&gt; getFutureStatsData() async {
    return await Statistics.calculateStatistics(allHabits);
  }
}
